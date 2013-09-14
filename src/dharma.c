/*******************************************************************************
*                            Dharma Clock v1.0                                 *
*                                                                              *
* Programmed by Miguel Branco (http://www.epicvortex.com)                      *
*                                                                              *
* Concept by Elzor (http://www.mypebblefaces.com/concepts/?auID=6&aName=Elzor) *
*                                                                              *
*******************************************************************************/


#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


// This should NEVER be done (except this time)
//#define MY_UUID { 0x59, 0x1E, 0x73, 0x00, 0xCD, 0x2D, 0x49, 0x9D, 0x99, 0x67, 0x60, 0x4B, 0x52, 0x32, 0x9B, 0xCB }
#define MY_UUID { 0x04, 0x08, 0x15, 0x16, 0x23, 0x42, 0x49, 0x9D, 0x99, 0x67, 0x60, 0x4B, 0x52, 0x32, 0x9B, 0xCB }
PBL_APP_INFO(MY_UUID,
			"Dharma Clock", "Miguel Branco",
			1, 0, 							// App version 
			RESOURCE_ID_IMAGE_MENU_ICON, 	// or DEFAULT_MENU_ICON,
			APP_INFO_WATCH_FACE); 			//or APP_INFO_STANDARD_APP);


/* Useful constants */
#define PBLWIDTH	144
#define PBLHEIGHT	168
#define STARTX		6
#define STARTXMIN	88
#define STARTY		127
#define DIGITWIDTH	24
#define DIGITHEIGHT	34

Window window;
BmpContainer dharma_container; 			// The dharma symbol
BmpContainer dividers_container;		// The dividers
BmpContainer flip_container[5];			// For the flipping animation
PropertyAnimation prop_animation[5];	// The animation itself
//RotBmpPairContainer dividersalpha_container;	// The alpha dividers
GFont custom_font;						// Custom font for the digits
TextLayer digitLayers[5];				// The clock digits layers
char digit[5][2];						// The clock digits to draw on the layer


/* Animation Handlers */

void startAnimation(Animation *animation, void *data) {
}

void endAnimation(Animation *animation, bool finished, void *data) {
	int index = (int) data;
	layer_remove_from_parent(&flip_container[index].layer.layer);
}

void scheduleAnimation(int i) {
	layer_add_child(&window.layer, &flip_container[i].layer.layer);
	animation_schedule(&prop_animation[i].animation);
}



/* Update the clock digits */
void update() {
	char timeText[] = "00:00:00";
	PblTm currentTime;
	get_time(&currentTime);
	if (clock_is_24h_style()) {
		string_format_time(timeText, sizeof(timeText), "%H:%M:%S", &currentTime);
	} else {
		string_format_time(timeText, sizeof(timeText), "%I:%M:%S", &currentTime);
	}
	
	digit[0][0] = '0';
	digit[1][0] = timeText[0];
	digit[2][0] = timeText[1];
	digit[3][0] = timeText[3];
	digit[4][0] = timeText[4];
	
	for (int i = 0; i < 5; ++i) {
		digit[i][1] = '\0';
		text_layer_set_text(&digitLayers[i], digit[i]);
	}
}

/* Tick handler (MINUTE/SECOND) */
void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
	(void)t;
	
	// Grab a copy of the current digits
	char current[5];
	for (int i = 0; i < 5; ++i) {
		current[i] = digit[i][0];
	}
	
	update();
	
	// Schedule animations for digits that changed
	for (int i = 0; i < 5; ++i) {
		if (current[i] != digit[i][0]) {
			scheduleAnimation(i);
		}
	}
}



void handle_init(AppContextRef ctx) {
	(void)ctx;
	window_init(&window, "Dharma Window");
	window_stack_push(&window, true /* Animated */);
	//window_set_background_color(&window, GColorBlack);
	
	// Init the resources
	resource_init_current_app(&DHARMA_RESOURCES);
	
	// Init the images and fonts
	bmp_init_container(RESOURCE_ID_IMAGE_DHARMA, &dharma_container);
	bmp_init_container(RESOURCE_ID_IMAGE_DIVIDERS, &dividers_container);
	layer_set_frame(&dividers_container.layer.layer, GRect(0, 144, PBLWIDTH, 3));
	bitmap_layer_set_compositing_mode(&dividers_container.layer, GCompOpAnd);	// Draw only the black pixels
	custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMPACT_30));
	
	// TODO Alpha dividers: would be great but transparent png support seems somewhat buggy
	//rotbmp_pair_init_container(RESOURCE_ID_IMAGE_DIVIDERS_ALPHA_WHITE, RESOURCE_ID_IMAGE_DIVIDERS_ALPHA_BLACK, &dividersalpha_container);
	//rotbmp_pair_layer_set_src_ic(&dividersalpha_container.layer, GPoint(20, 1));
	
	
	// Animation Rects
	GRect from_rect[5];
	from_rect[0] = GRect(STARTX, STARTY+1, 24, 10);
	from_rect[1] = GRect(STARTX+DIGITWIDTH+2, STARTY+1, 24, 10);
	from_rect[2] = GRect(STARTX+(DIGITWIDTH*2)+4, STARTY+1, 24, 10);
	from_rect[3] = GRect(STARTXMIN, STARTY+1, 24, 10);
	from_rect[4] = GRect(STARTXMIN+DIGITWIDTH+2, STARTY+1, 24, 10);
	
	for (int i = 0; i < 5; ++i) {
		bmp_init_container(RESOURCE_ID_IMAGE_FLIP, &flip_container[i]);
		layer_set_frame(&flip_container[i].layer.layer, from_rect[i]);
	}
		
	// Animations
	AnimationHandlers handlers = {
		.started = &startAnimation,
		.stopped = &endAnimation
	};
	
	for (int i = 0; i < 5; ++i) {
		GRect to_rect = from_rect[i];
		to_rect.origin.y = 152;
		property_animation_init_layer_frame(&prop_animation[i], &flip_container[i].layer.layer, &from_rect[i], &to_rect);
		animation_set_handlers(&prop_animation[i].animation, handlers, (void*) i);
		animation_set_duration(&prop_animation[i].animation, 100);
	}
	
	
	// Time Digit Layers	
	for (int i = 0; i < 5; ++i) {
		from_rect[i] = GRect(from_rect[i].origin.x, from_rect[i].origin.y-1, DIGITWIDTH, DIGITHEIGHT);
		text_layer_init(&digitLayers[i], from_rect[i]);
		text_layer_set_text_color(&digitLayers[i], (i<3? GColorWhite : GColorBlack) );
		text_layer_set_background_color(&digitLayers[i], GColorClear);
		text_layer_set_font(&digitLayers[i], custom_font);
		text_layer_set_text_alignment(&digitLayers[i], GTextAlignmentCenter);
	}
	
	
	// Update immediately when initialized
	update();


	// Add layers
	layer_add_child(&window.layer, &dharma_container.layer.layer);
	
	for (int i = 0; i < 5; ++i) {
		layer_add_child(&window.layer, &digitLayers[i].layer);
	}

	layer_add_child(&window.layer, &dividers_container.layer.layer);
	//layer_add_child(&window.layer, &dividersalpha_container.layer.layer);
}


void handle_deinit(AppContextRef ctx) {
	(void)ctx;
	
	// Unload images
	bmp_deinit_container(&dharma_container);
	bmp_deinit_container(&dividers_container);
	for (int i = 0; i < 5; ++i) {
		bmp_deinit_container(&flip_container[i]);
	}
	//rotbmp_pair_deinit_container(&dividersalpha_container);
	
	// Unload fonts
	fonts_unload_custom_font(custom_font);
}

/* Watchface entry point - set handlers for init, deinit and tick (SECOND/MINUTE) */
void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		
		.tick_info = {
			.tick_handler = &handle_tick,
			.tick_units = MINUTE_UNIT
		}
	};
	app_event_loop(params, &handlers);
}







