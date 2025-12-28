#if defined(PBL_RECT)
#include "line_window.h"

#define LOGO_RADIUS scalable_x(85)
#define STRIPE_WIDTH scalable_x_pp(85, 90)
#define LOGO_MARGIN scalable_x_pp(80, 80)

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static bool s_good_lines_expanded = false;

/******************************** Click config *********************************/

static void select_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int position = (int)cell_index->row;
  int disrupted_count = data_get_disrupted_or_pinned_count();
  
  // Check if this is the "All other lines" row
  if (!s_good_lines_expanded && position == disrupted_count) {
    s_good_lines_expanded = true;
    menu_layer_reload_data(s_menu_layer);
    return;
  }
  
  // Adjust position if expanded and past the divider
  int actual_position = position;
  if (s_good_lines_expanded && position > disrupted_count) {
    actual_position = position - 1;  // Skip the divider row
  }
  
  int index = data_get_line_index_at_position(actual_position);
  if (!data_get_line_has_reason(index)) return;

  reason_window_push(index);
}

static void select_long_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int position = (int)cell_index->row;
  int disrupted_count = data_get_disrupted_or_pinned_count();
  
  // Ignore long click on "All other lines" row
  if (!s_good_lines_expanded && position == disrupted_count) {
    return;
  }
  
  // Adjust position if expanded and past the divider
  int actual_position = position;
  if (s_good_lines_expanded && position > disrupted_count) {
    actual_position = position - 1;
  }
  
  int index = data_get_line_index_at_position(actual_position);
  
  data_toggle_line_pinned(index);
  
  // Refresh the menu layer
  menu_layer_reload_data(s_menu_layer);
  
  // Vibrate to confirm
  vibes_short_pulse();
}

/********************************* MenuLayer **********************************/

void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  int position = cell_index->row;
  int disrupted_count = data_get_disrupted_or_pinned_count();
  
  // Draw "All other lines" divider row
  if (position == disrupted_count) {
    graphics_context_set_fill_color(ctx, GColorLightGray);
    graphics_fill_rect(ctx, bounds, GCornerNone, 0);
    
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(
      ctx,
      s_good_lines_expanded ? "All other lines" : "All other lines  >",
      scalable_get_font(SFI_MediumBold),
      scalable_grect_pp(
        GRect(40, 10, 900, 200),
        GRect(50, 25, 900, 200)
      ),
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft,
      NULL
    );
    
    // Sep
    const int sep_h = scalable_y_pp(10, 15);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.size.h - sep_h, bounds.size.w, sep_h), GCornerNone, 0);
    return;
  }
  
  // Adjust position for rows after the divider
  int actual_position = position;
  if (position > disrupted_count) {
    actual_position = position - 1;
  }

  int index = data_get_line_index_at_position(actual_position);
  LineData *line_data = data_get_line(index);
  
  // Determine if this line has good service
  bool has_good_service = (strlen(line_data->state) == 0);

  const int logo_bounds_x = bounds.origin.x + (LOGO_MARGIN / 4);
  GRect logo_bounds = GRect(
    logo_bounds_x,
    (bounds.size.h - (2 * LOGO_RADIUS)) / 2,
    (2 * LOGO_RADIUS),
    (2 * LOGO_RADIUS)
  );
  GPoint center = grect_center_point(&logo_bounds);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, GCornerNone, 0);

  // Line color
  int line_color_x = center.x - (STRIPE_WIDTH / 2);
  graphics_context_set_fill_color(ctx, data_get_line_color(line_data->type));
  graphics_fill_rect(
    ctx,
    GRect(line_color_x, bounds.origin.y, STRIPE_WIDTH, bounds.size.h),
    GCornerNone,
    0
  );
  if (data_get_line_color_is_striped(line_data->type)) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(
      ctx,
      GRect(line_color_x + (STRIPE_WIDTH / 3), bounds.origin.y, STRIPE_WIDTH / 3, bounds.size.h),
      GCornerNone,
      0
    );
  }

  // Draw circle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, LOGO_RADIUS);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, (5 * (LOGO_RADIUS)) / 7);

  // Show selected
  if (menu_layer_is_index_selected(s_menu_layer, cell_index)) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, center, (4 * (LOGO_RADIUS - 2)) / 7);
  }

  // Name and status
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(
    ctx,
    data_get_line_name(line_data->type),
    scalable_get_font(SFI_Medium),
    scalable_grect_pp(
      GRect(220, -40, 750, 200),
      GRect(230, -20, 750, 200)
    ),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
  graphics_draw_text(
    ctx,
    has_good_service ? "Good Service" : line_data->state,
    scalable_get_font(SFI_MediumBold),
    scalable_grect_pp(
      GRect(220, 75, 750, 200),
      GRect(230, 95, 750, 200)
    ),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );

  // Show hint arrow if reason can be viewed
  if (menu_layer_is_index_selected(s_menu_layer, cell_index) && data_get_line_has_reason(index)) {
    // Background
    graphics_context_set_fill_color(ctx, data_get_line_state_color(index));
    graphics_fill_rect(
      ctx,
      scalable_grect(915, 0, 90, 260),
      GCornerNone,
      0
    );
    
    // Arrow
    graphics_draw_text(
      ctx,
      ">",
      scalable_get_font(SFI_MediumBold),
      scalable_grect_pp(
        GRect(930, 25, 120, 500),
        GRect(935, 40, 120, 500)
      ),
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft,
      NULL
    );
  }
  
  if (data_is_line_pinned(index)) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(
      ctx,
      "*",
      scalable_get_font(SFI_Small),
      scalable_grect_pp(
        GRect(180, 10, 200, 100),
        GRect(190, 10, 200, 100)
      ),
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft,
      NULL
    );
  }

  // Sep
  const int sep_h = scalable_y_pp(10, 15);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.size.h - sep_h, bounds.size.w, sep_h), GCornerNone, 0);
}

uint16_t get_num_rows_handler(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  int disrupted_count = data_get_disrupted_or_pinned_count();
  if (s_good_lines_expanded) {
    // All lines + 1 divider row
    return LineTypeMax + 1;
  } else {
    // Disrupted/pinned lines + 1 "All other lines" row
    return disrupted_count + 1;
  }
}

int16_t get_cell_height_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int disrupted_count = data_get_disrupted_or_pinned_count();
  // Smaller height for the "All other lines" divider row
  if ((int)cell_index->row == disrupted_count) {
    return scalable_y(180);
  }
  return scalable_y(260);
}

void selection_will_change_handler(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, void *context) {
  // Allow selection of all lines
}

/*********************************** Window ***********************************/

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);

  GEdgeInsets menu_insets = (GEdgeInsets) { .top = STATUS_BAR_LAYER_HEIGHT - 1 };
  GRect menu_bounds = grect_inset(bounds, menu_insets);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  s_menu_layer = menu_layer_create(menu_bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
  menu_layer_pad_bottom_enable(s_menu_layer, false);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .draw_row = draw_row_handler,
    .get_num_rows = get_num_rows_handler,
    .get_cell_height = get_cell_height_handler,
    .select_click = select_click_handler,
    .select_long_click = select_long_click_handler,
    .selection_will_change = selection_will_change_handler,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);

  window_destroy(s_window);
  s_window = NULL;
  window_stack_pop_all(true);  // Don't show splash on exit
}

void line_window_push() {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_window, true);
}
#endif
