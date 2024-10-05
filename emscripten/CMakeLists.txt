cmake_minimum_required(VERSION 3.4.1)
project(LibreSprite C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DEMSCRIPTEN -DHAVE_CONFIG_H)
set(EMSCRIPTEN on)

set(USE_SDL2_BACKEND on)

set(LOADPNG_DIR         ${CMAKE_SOURCE_DIR}/../third_party/loadpng)
set(SIMPLEINI_DIR       ${CMAKE_SOURCE_DIR}/../third_party/simpleini)
set(DUKTAPE_DIR         ${CMAKE_SOURCE_DIR}/../third_party/duktape)
set(MODP_B64_DIR        ${CMAKE_SOURCE_DIR}/../third_party/modp_b64)
set(QOI_DIR             ${CMAKE_SOURCE_DIR}/../third_party/qoi)


include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../android/tinyxml2
        ${CMAKE_CURRENT_SOURCE_DIR}/../android/pixman/pixman
        ${CMAKE_CURRENT_SOURCE_DIR}/../android/pixman
        ${SIMPLEINI_DIR}
        ${DUKTAPE_DIR}
        ${MODP_B64_DIR}
        ${QOI_DIR}
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ ${CMAKE_CURRENT_SOURCE_DIR}/../build/third_party/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../android/tinyxml2 ${CMAKE_CURRENT_SOURCE_DIR}/../build/tinyxml2/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../android/pixman/ ${CMAKE_CURRENT_SOURCE_DIR}/../build/pixman/)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(GENSRC ${CMAKE_CURRENT_SOURCE_DIR}/../build/src)

include_directories(
  ${SRC}
  ${GENSRC}/base
  ${SRC}/observable
  ${GENSRC}/app
)

set(USE_FLAGS "-O3 -pthread -s USE_PTHREADS=1 -flto -fwasm-exceptions -s SUPPORT_LONGJMP=wasm")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
include_directories(${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -sPTHREAD_POOL_SIZE=4 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s USE_FREETYPE=1 -s USE_ZLIB=1 -s USE_GIFLIB=1 -s USE_LIBJPEG=1 -s USE_LIBPNG -s SDL2_IMAGE_FORMATS='[\"png\",\"jpg\"]' -Wl,-u,fileno -Wl,-u,_emscripten_run_callback_on_thread --preload-file ../../build/bin/data@/data")
set(CMAKE_EXECUTABLE_SUFFIX .html)

set_source_files_properties(${SRC}/app/file/ase_format.cpp PROPERTIES COMPILE_FLAGS "-s USE_ZLIB=1")
set_source_files_properties(${SRC}/app/file/gif_format.cpp PROPERTIES COMPILE_FLAGS "-s USE_GIFLIB=1")
set_source_files_properties(${SRC}/app/file/jpeg_format.cpp PROPERTIES COMPILE_FLAGS "-s USE_LIBJPEG=1")
set_source_files_properties(${SRC}/app/file/png_format.cpp PROPERTIES COMPILE_FLAGS "-s USE_LIBPNG=1")
set_source_files_properties(${SRC}/app/util/freetype_utils.cpp PROPERTIES COMPILE_FLAGS "-s USE_FREETYPE=1")
set_source_files_properties(${SRC}/she/common/freetype_font.cpp PROPERTIES COMPILE_FLAGS "-s USE_FREETYPE=1")
set_source_files_properties(${SRC}/she/sdl2/sdl2_display.cpp PROPERTIES COMPILE_FLAGS "-s USE_FREETYPE=1 -s USE_SDL=2")
set_source_files_properties(${SRC}/she/sdl2/sdl2_surface.cpp PROPERTIES COMPILE_FLAGS "-s USE_FREETYPE=1 -s USE_SDL=2")
set_source_files_properties(${SRC}/she/sdl2/she.cpp PROPERTIES COMPILE_FLAGS "-s USE_FREETYPE=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"png\",\"jpg\"]'")


set(SRC_FILES
  ${GENSRC}/app/pref.xml.cpp
  ${DUKTAPE_DIR}/duktape.c
  # ${SRC}/app/color_tests.cpp
  # ${SRC}/app/document_api_tests.cpp
  # ${SRC}/app/document_range_tests.cpp
  # ${SRC}/app/errno_tests.cpp
  # ${SRC}/app/file/file_tests.cpp
  # ${SRC}/app/file/split_filename_tests.cpp
  ${SRC}/app/file/webp_format.cpp
  # ${SRC}/app/filename_formatter_tests.cpp
  # ${SRC}/app/ini_file_tests.cpp
  # ${SRC}/base/base64_tests.cpp
  # ${SRC}/base/file_handle_tests.cpp
  # ${SRC}/base/fs_tests.cpp
  # ${SRC}/base/path_tests.cpp
  # ${SRC}/base/program_options_tests.cpp
  # ${SRC}/base/replace_string_tests.cpp
  # ${SRC}/base/shared_ptr_tests.cpp
  # ${SRC}/base/split_string_tests.cpp
  # ${SRC}/base/string_tests.cpp
  # ${SRC}/base/thread_tests.cpp
  # ${SRC}/base/version_tests.cpp
  # ${SRC}/base/win32_exception.cpp
  # ${SRC}/clip/clip_win.cpp
  # ${SRC}/clip/clip_x11.cpp
  # ${SRC}/clip/examples/copy.cpp
  # ${SRC}/clip/examples/helloworld.cpp
  # ${SRC}/clip/examples/int_format.cpp
  # ${SRC}/clip/examples/paste.cpp
  # ${SRC}/clip/examples/put_image.cpp
  # ${SRC}/clip/examples/show_image.cpp
  # ${SRC}/clip/tests/image_tests.cpp
  # ${SRC}/clip/tests/text_tests.cpp
  # ${SRC}/clip/tests/user_format_tests.cpp
  # ${SRC}/css/css_tests.cpp
  # ${SRC}/doc/context_tests.cpp
  # ${SRC}/doc/image_tests.cpp
  # ${SRC}/doc/remap_tests.cpp
  # ${SRC}/doc/resize_image_tests.cpp
  # ${SRC}/doc/sprite_tests.cpp
  # ${SRC}/gen/gen.cpp
  # ${SRC}/gen/pref_types.cpp
  # ${SRC}/gen/skin_class.cpp
  # ${SRC}/gen/ui_class.cpp
  # ${SRC}/gfx/clip_tests.cpp
  # ${SRC}/gfx/hsv_tests.cpp
  # ${SRC}/gfx/packing_rects_tests.cpp
  # ${SRC}/gfx/rect_tests.cpp
  # ${SRC}/gfx/region_tests.cpp
  # ${SRC}/gfx/rgb_tests.cpp
  # ${SRC}/observable/tests/disconnect_on_dtor.cpp
  # ${SRC}/observable/tests/disconnect_on_rescursive_signal.cpp
  # ${SRC}/observable/tests/disconnect_on_signal.cpp
  # ${SRC}/observable/tests/multithread.cpp
  # ${SRC}/observable/tests/observers.cpp
  # ${SRC}/observable/tests/reconnect_on_notification.cpp
  # ${SRC}/observable/tests/reconnect_on_signal.cpp
  # ${SRC}/observable/tests/signals.cpp
  # ${SRC}/render/ordered_dither_tests.cpp
  # ${SRC}/render/render_tests.cpp
  # ${SRC}/script/lua/engine.cpp
  # ${SRC}/script/v8/engine.cpp
  # ${SRC}/she/alleg4/alleg_display.cpp
  # ${SRC}/she/alleg4/alleg_surface.cpp
  # ${SRC}/she/alleg4/display_events.cpp
  # ${SRC}/she/alleg4/key_poller.cpp
  # ${SRC}/she/alleg4/mouse_poller.cpp
  # ${SRC}/she/alleg4/she.cpp
  # ${SRC}/she/gtk/native_dialogs.cpp
  # ${SRC}/she/win/native_dialogs.cpp
  # ${SRC}/she/win/pen.cpp
  # ${SRC}/she/win/vk.cpp
  # ${SRC}/she/win/window_dde.cpp
  # ${SRC}/ui/accelerator_ui_tests.cpp
  # ${SRC}/ui/grid_ui_tests.cpp
  # ${SRC}/undo/tests/basics.cpp
  # ${SRC}/undo/tests/complex_tree.cpp
  # ${SRC}/undo/tests/tree.cpp
  ${SRC}/app/app.cpp
  ${SRC}/app/app_brushes.cpp
  ${SRC}/app/app_menus.cpp
  ${SRC}/app/app_options.cpp
  ${SRC}/app/app_render.cpp
  ${SRC}/app/cmd.cpp
  ${SRC}/app/cmd/add_cel.cpp
  ${SRC}/app/cmd/add_frame.cpp
  ${SRC}/app/cmd/add_frame_tag.cpp
  ${SRC}/app/cmd/add_layer.cpp
  ${SRC}/app/cmd/add_palette.cpp
  ${SRC}/app/cmd/background_from_layer.cpp
  ${SRC}/app/cmd/clear_cel.cpp
  ${SRC}/app/cmd/clear_image.cpp
  ${SRC}/app/cmd/clear_mask.cpp
  ${SRC}/app/cmd/clear_rect.cpp
  ${SRC}/app/cmd/configure_background.cpp
  ${SRC}/app/cmd/copy_cel.cpp
  ${SRC}/app/cmd/copy_frame.cpp
  ${SRC}/app/cmd/copy_rect.cpp
  ${SRC}/app/cmd/copy_region.cpp
  ${SRC}/app/cmd/crop_cel.cpp
  ${SRC}/app/cmd/deselect_mask.cpp
  ${SRC}/app/cmd/flatten_layers.cpp
  ${SRC}/app/cmd/flip_image.cpp
  ${SRC}/app/cmd/flip_mask.cpp
  ${SRC}/app/cmd/flip_masked_cel.cpp
  ${SRC}/app/cmd/layer_from_background.cpp
  ${SRC}/app/cmd/move_cel.cpp
  ${SRC}/app/cmd/move_layer.cpp
  ${SRC}/app/cmd/patch_cel.cpp
  ${SRC}/app/cmd/remap_colors.cpp
  ${SRC}/app/cmd/remove_cel.cpp
  ${SRC}/app/cmd/remove_frame.cpp
  ${SRC}/app/cmd/remove_frame_tag.cpp
  ${SRC}/app/cmd/remove_layer.cpp
  ${SRC}/app/cmd/remove_palette.cpp
  ${SRC}/app/cmd/replace_image.cpp
  ${SRC}/app/cmd/reselect_mask.cpp
  ${SRC}/app/cmd/set_cel_data.cpp
  ${SRC}/app/cmd/set_cel_frame.cpp
  ${SRC}/app/cmd/set_cel_opacity.cpp
  ${SRC}/app/cmd/set_cel_position.cpp
  ${SRC}/app/cmd/set_frame_duration.cpp
  ${SRC}/app/cmd/set_frame_tag_anidir.cpp
  ${SRC}/app/cmd/set_frame_tag_color.cpp
  ${SRC}/app/cmd/set_frame_tag_name.cpp
  ${SRC}/app/cmd/set_frame_tag_range.cpp
  ${SRC}/app/cmd/set_layer_blend_mode.cpp
  ${SRC}/app/cmd/set_layer_flags.cpp
  ${SRC}/app/cmd/set_layer_name.cpp
  ${SRC}/app/cmd/set_layer_opacity.cpp
  ${SRC}/app/cmd/set_mask.cpp
  ${SRC}/app/cmd/set_mask_position.cpp
  ${SRC}/app/cmd/set_palette.cpp
  ${SRC}/app/cmd/set_pixel_format.cpp
  ${SRC}/app/cmd/set_sprite_size.cpp
  ${SRC}/app/cmd/set_total_frames.cpp
  ${SRC}/app/cmd/set_transparent_color.cpp
  ${SRC}/app/cmd/set_user_data.cpp
  ${SRC}/app/cmd/shift_masked_cel.cpp
  ${SRC}/app/cmd/trim_cel.cpp
  ${SRC}/app/cmd/unlink_cel.cpp
  ${SRC}/app/cmd/with_cel.cpp
  ${SRC}/app/cmd/with_document.cpp
  ${SRC}/app/cmd/with_frame_tag.cpp
  ${SRC}/app/cmd/with_image.cpp
  ${SRC}/app/cmd/with_layer.cpp
  ${SRC}/app/cmd/with_sprite.cpp
  ${SRC}/app/cmd_sequence.cpp
  ${SRC}/app/cmd_transaction.cpp
  ${SRC}/app/color.cpp
  ${SRC}/app/color_picker.cpp
  ${SRC}/app/color_utils.cpp
  ${SRC}/app/commands/cmd_about.cpp
  ${SRC}/app/commands/cmd_add_color.cpp
  ${SRC}/app/commands/cmd_advanced_mode.cpp
  ${SRC}/app/commands/cmd_alternate_timeline.cpp
  ${SRC}/app/commands/cmd_alternate_toolbar.cpp
  ${SRC}/app/commands/cmd_alternate_touchbar.cpp
  ${SRC}/app/commands/cmd_background_from_layer.cpp
  ${SRC}/app/commands/cmd_cancel.cpp
  ${SRC}/app/commands/cmd_canvas_size.cpp
  ${SRC}/app/commands/cmd_cel_properties.cpp
  ${SRC}/app/commands/cmd_change_brush.cpp
  ${SRC}/app/commands/cmd_change_color.cpp
  ${SRC}/app/commands/cmd_change_pixel_format.cpp
  ${SRC}/app/commands/cmd_clear.cpp
  ${SRC}/app/commands/cmd_clear_cel.cpp
  ${SRC}/app/commands/cmd_close_file.cpp
  ${SRC}/app/commands/cmd_color_quantization.cpp
  ${SRC}/app/commands/cmd_copy.cpp
  ${SRC}/app/commands/cmd_copy_cel.cpp
  ${SRC}/app/commands/cmd_copy_merged.cpp
  ${SRC}/app/commands/cmd_crop.cpp
  ${SRC}/app/commands/cmd_cut.cpp
  ${SRC}/app/commands/cmd_deselect_mask.cpp
  ${SRC}/app/commands/cmd_developer_console.cpp
  ${SRC}/app/commands/cmd_discard_brush.cpp
  ${SRC}/app/commands/cmd_duplicate_layer.cpp
  ${SRC}/app/commands/cmd_duplicate_sprite.cpp
  ${SRC}/app/commands/cmd_duplicate_view.cpp
  ${SRC}/app/commands/cmd_exit.cpp
  ${SRC}/app/commands/cmd_export_sprite_sheet.cpp
  ${SRC}/app/commands/cmd_eyedropper.cpp
  ${SRC}/app/commands/cmd_flatten_layers.cpp
  ${SRC}/app/commands/cmd_flip.cpp
  ${SRC}/app/commands/cmd_frame_properties.cpp
  ${SRC}/app/commands/cmd_frame_tag_properties.cpp
  ${SRC}/app/commands/cmd_fullscreen_preview.cpp
  ${SRC}/app/commands/cmd_goto_frame.cpp
  ${SRC}/app/commands/cmd_goto_layer.cpp
  ${SRC}/app/commands/cmd_goto_tab.cpp
  ${SRC}/app/commands/cmd_grid.cpp
  ${SRC}/app/commands/cmd_home.cpp
  ${SRC}/app/commands/cmd_import_sprite_sheet.cpp
  ${SRC}/app/commands/cmd_invert_mask.cpp
  ${SRC}/app/commands/cmd_keyboard_shortcuts.cpp
  ${SRC}/app/commands/cmd_launch.cpp
  ${SRC}/app/commands/cmd_layer_from_background.cpp
  ${SRC}/app/commands/cmd_layer_properties.cpp
  ${SRC}/app/commands/cmd_layer_visibility.cpp
  ${SRC}/app/commands/cmd_link_cels.cpp
  ${SRC}/app/commands/cmd_load_mask.cpp
  ${SRC}/app/commands/cmd_load_palette.cpp
  ${SRC}/app/commands/cmd_mask_all.cpp
  ${SRC}/app/commands/cmd_mask_by_color.cpp
  ${SRC}/app/commands/cmd_mask_content.cpp
  ${SRC}/app/commands/cmd_merge_down_layer.cpp
  ${SRC}/app/commands/cmd_modify_selection.cpp
  ${SRC}/app/commands/cmd_move_cel.cpp
  ${SRC}/app/commands/cmd_move_mask.cpp
  ${SRC}/app/commands/cmd_new_brush.cpp
  ${SRC}/app/commands/cmd_new_file.cpp
  ${SRC}/app/commands/cmd_new_frame.cpp
  ${SRC}/app/commands/cmd_new_frame_tag.cpp
  ${SRC}/app/commands/cmd_new_layer.cpp
  ${SRC}/app/commands/cmd_new_layer_set.cpp
  ${SRC}/app/commands/cmd_new_sprite_from_selection.cpp
  ${SRC}/app/commands/cmd_onionskin.cpp
  ${SRC}/app/commands/cmd_open_file.cpp
  ${SRC}/app/commands/cmd_open_in_folder.cpp
  ${SRC}/app/commands/cmd_open_scripts_folder.cpp
  ${SRC}/app/commands/cmd_open_with_app.cpp
  ${SRC}/app/commands/cmd_options.cpp
  ${SRC}/app/commands/cmd_palette_editor.cpp
  ${SRC}/app/commands/cmd_palette_size.cpp
  ${SRC}/app/commands/cmd_paste.cpp
  ${SRC}/app/commands/cmd_paste_text.cpp
  ${SRC}/app/commands/cmd_pixel_perfect_mode.cpp
  ${SRC}/app/commands/cmd_play_animation.cpp
  ${SRC}/app/commands/cmd_refresh.cpp
  ${SRC}/app/commands/cmd_remove_frame.cpp
  ${SRC}/app/commands/cmd_remove_frame_tag.cpp
  ${SRC}/app/commands/cmd_remove_layer.cpp
  ${SRC}/app/commands/cmd_repeat_last_export.cpp
  ${SRC}/app/commands/cmd_rescan_scripts.cpp
  ${SRC}/app/commands/cmd_reselect_mask.cpp
  ${SRC}/app/commands/cmd_reverse_frames.cpp
  ${SRC}/app/commands/cmd_rotate.cpp
  ${SRC}/app/commands/cmd_run_script.cpp
  ${SRC}/app/commands/cmd_save_file.cpp
  ${SRC}/app/commands/cmd_save_mask.cpp
  ${SRC}/app/commands/cmd_save_palette.cpp
  ${SRC}/app/commands/cmd_scroll.cpp
  ${SRC}/app/commands/cmd_scroll_center.cpp
  ${SRC}/app/commands/cmd_select_tile.cpp
  ${SRC}/app/commands/cmd_set_color_selector.cpp
  ${SRC}/app/commands/cmd_set_ink_type.cpp
  ${SRC}/app/commands/cmd_set_loop_section.cpp
  ${SRC}/app/commands/cmd_set_palette.cpp
  ${SRC}/app/commands/cmd_set_palette_entry_size.cpp
  ${SRC}/app/commands/cmd_set_same_ink.cpp
  ${SRC}/app/commands/cmd_share.cpp
  ${SRC}/app/commands/cmd_show.cpp
  ${SRC}/app/commands/cmd_sprite_properties.cpp
  ${SRC}/app/commands/cmd_sprite_size.cpp
  ${SRC}/app/commands/cmd_switch_colors.cpp
  ${SRC}/app/commands/cmd_symmetry_mode.cpp
  ${SRC}/app/commands/cmd_tiled_mode.cpp
  ${SRC}/app/commands/cmd_timeline.cpp
  ${SRC}/app/commands/cmd_toggle_fullscreen.cpp
  ${SRC}/app/commands/cmd_toggle_preview.cpp
  ${SRC}/app/commands/cmd_toggle_touchbar.cpp
  ${SRC}/app/commands/cmd_undo.cpp
  ${SRC}/app/commands/cmd_undo_history.cpp
  ${SRC}/app/commands/cmd_unlink_cel.cpp
  ${SRC}/app/commands/cmd_zoom.cpp
  ${SRC}/app/commands/command.cpp
  ${SRC}/app/commands/commands.cpp
  ${SRC}/app/commands/filters/cmd_color_curve.cpp
  ${SRC}/app/commands/filters/cmd_convolution_matrix.cpp
  ${SRC}/app/commands/filters/cmd_despeckle.cpp
  ${SRC}/app/commands/filters/cmd_invert_color.cpp
  ${SRC}/app/commands/filters/cmd_replace_color.cpp
  ${SRC}/app/commands/filters/color_curve_editor.cpp
  ${SRC}/app/commands/filters/convolution_matrix_stock.cpp
  ${SRC}/app/commands/filters/filter_manager_impl.cpp
  ${SRC}/app/commands/filters/filter_preview.cpp
  ${SRC}/app/commands/filters/filter_target_buttons.cpp
  ${SRC}/app/commands/filters/filter_window.cpp
  ${SRC}/app/commands/filters/filter_worker.cpp
  ${SRC}/app/console.cpp
  ${SRC}/app/context.cpp
  ${SRC}/app/context_flags.cpp
  ${SRC}/app/crash/backup_observer.cpp
  ${SRC}/app/crash/data_recovery.cpp
  ${SRC}/app/crash/read_document.cpp
  ${SRC}/app/crash/session.cpp
  ${SRC}/app/crash/write_document.cpp
  ${SRC}/app/document.cpp
  ${SRC}/app/document_api.cpp
  ${SRC}/app/document_exporter.cpp
  ${SRC}/app/document_range.cpp
  ${SRC}/app/document_range_ops.cpp
  ${SRC}/app/document_undo.cpp
  ${SRC}/app/extra_cel.cpp
  ${SRC}/app/file/ase_format.cpp
  ${SRC}/app/file/bmp_format.cpp
  ${SRC}/app/file/extension_format.cpp
  ${SRC}/app/file/file.cpp
  ${SRC}/app/file/file_format.cpp
  ${SRC}/app/file/file_formats_manager.cpp
  ${SRC}/app/file/fli_format.cpp
  ${SRC}/app/file/gif_format.cpp
  ${SRC}/app/file/ico_format.cpp
  ${SRC}/app/file/jpeg_format.cpp
  ${SRC}/app/file/palette_file.cpp
  ${SRC}/app/file/pcx_format.cpp
  ${SRC}/app/file/pixly_format.cpp
  ${SRC}/app/file/png_format.cpp
  ${SRC}/app/file/she_format.cpp
  ${SRC}/app/file/split_filename.cpp
  ${SRC}/app/file/tga_format.cpp
  ${SRC}/app/file_selector.cpp
  ${SRC}/app/file_system.cpp
  ${SRC}/app/filename_formatter.cpp
  ${SRC}/app/flatten.cpp
  ${SRC}/app/gui_xml.cpp
  ${SRC}/app/ini_file.cpp
  ${SRC}/app/job.cpp
  ${SRC}/app/launcher.cpp
  ${SRC}/app/log.cpp
  ${SRC}/app/loop_tag.cpp
  ${SRC}/app/modules.cpp
  ${SRC}/app/modules/editors.cpp
  ${SRC}/app/modules/gfx.cpp
  ${SRC}/app/modules/gui.cpp
  ${SRC}/app/modules/i18n.cpp
  ${SRC}/app/modules/palettes.cpp
  ${SRC}/app/pref/preferences.cpp
  ${SRC}/app/project.cpp
  ${SRC}/app/recent_files.cpp
  ${SRC}/app/recent_files_menu.cpp
  ${SRC}/app/res/palettes_loader_delegate.cpp
  ${SRC}/app/res/resources_loader.cpp
  ${SRC}/app/resource_finder.cpp
  ${SRC}/app/script/api/app_script.cpp
  ${SRC}/app/script/api/button_script.cpp
  ${SRC}/app/script/api/cel_script.cpp
  ${SRC}/app/script/api/colormode_script.cpp
  ${SRC}/app/script/api/console_script.cpp
  ${SRC}/app/script/api/command_script.cpp
  ${SRC}/app/script/api/dialog_script.cpp
  ${SRC}/app/script/api/document_script.cpp
  ${SRC}/app/script/api/entry_script.cpp
  ${SRC}/app/script/api/image_script.cpp
  ${SRC}/app/script/api/imageview_script.cpp
  ${SRC}/app/script/api/intentry_script.cpp
  ${SRC}/app/script/api/label_script.cpp
  ${SRC}/app/script/api/layer_script.cpp
  ${SRC}/app/script/api/palette_script.cpp
  ${SRC}/app/script/api/palettelistbox_script.cpp
  ${SRC}/app/script/api/pixelcolor_script.cpp
  ${SRC}/app/script/api/selection_script.cpp
  ${SRC}/app/script/api/sprite_script.cpp
  ${SRC}/app/script/api/storage_script.cpp
  ${SRC}/app/script/app_scripting.cpp
  ${SRC}/app/script/console_delegate.cpp
  ${SRC}/app/script/script_menu.cpp
  ${SRC}/app/send_crash.cpp
  ${SRC}/app/shade.cpp
  ${SRC}/app/shell.cpp
  ${SRC}/app/snap_to_grid.cpp
  ${SRC}/app/thumbnail_generator.cpp
  ${SRC}/app/tools/active_tool.cpp
  ${SRC}/app/tools/ink_type.cpp
  ${SRC}/app/tools/intertwine.cpp
  ${SRC}/app/tools/pick_ink.cpp
  ${SRC}/app/tools/point_shape.cpp
  ${SRC}/app/tools/stroke.cpp
  ${SRC}/app/tools/symmetry.cpp
  ${SRC}/app/tools/tool_box.cpp
  ${SRC}/app/tools/tool_loop_manager.cpp
  ${SRC}/app/transaction.cpp
  ${SRC}/app/transformation.cpp
  ${SRC}/app/ui/ani_controls.cpp
  ${SRC}/app/ui/app_menuitem.cpp
  ${SRC}/app/ui/brush_popup.cpp
  ${SRC}/app/ui/button_set.cpp
  ${SRC}/app/ui/color_bar.cpp
  ${SRC}/app/ui/color_button.cpp
  ${SRC}/app/ui/color_popup.cpp
  ${SRC}/app/ui/color_selector.cpp
  ${SRC}/app/ui/color_sliders.cpp
  ${SRC}/app/ui/color_spectrum.cpp
  ${SRC}/app/ui/color_tint_shade_tone.cpp
  ${SRC}/app/ui/color_wheel.cpp
  ${SRC}/app/ui/configure_timeline_popup.cpp
  ${SRC}/app/ui/context_bar.cpp
  ${SRC}/app/ui/data_recovery_view.cpp
  ${SRC}/app/ui/devconsole_view.cpp
  ${SRC}/app/ui/document_view.cpp
  ${SRC}/app/ui/drop_down_button.cpp
  ${SRC}/app/ui/editor/brush_preview.cpp
  ${SRC}/app/ui/editor/drawing_state.cpp
  ${SRC}/app/ui/editor/editor.cpp
  ${SRC}/app/ui/editor/editor_observers.cpp
  ${SRC}/app/ui/editor/editor_states_history.cpp
  ${SRC}/app/ui/editor/editor_view.cpp
  ${SRC}/app/ui/editor/moving_cel_state.cpp
  ${SRC}/app/ui/editor/moving_pixels_state.cpp
  ${SRC}/app/ui/editor/moving_symmetry_state.cpp
  ${SRC}/app/ui/editor/navigate_state.cpp
  ${SRC}/app/ui/editor/pivot_helpers.cpp
  ${SRC}/app/ui/editor/pixels_movement.cpp
  ${SRC}/app/ui/editor/play_state.cpp
  ${SRC}/app/ui/editor/scrolling_state.cpp
  ${SRC}/app/ui/editor/select_box_state.cpp
  ${SRC}/app/ui/editor/standby_state.cpp
  ${SRC}/app/ui/editor/state_with_wheel_behavior.cpp
  ${SRC}/app/ui/editor/tool_loop_impl.cpp
  ${SRC}/app/ui/editor/transform_handles.cpp
  ${SRC}/app/ui/editor/zooming_state.cpp
  ${SRC}/app/ui/file_list.cpp
  ${SRC}/app/ui/file_list_view.cpp
  ${SRC}/app/ui/file_selector.cpp
  ${SRC}/app/ui/font_popup.cpp
  ${SRC}/app/ui/frame_tag_window.cpp
  ${SRC}/app/ui/hex_color_entry.cpp
  ${SRC}/app/ui/home_view.cpp
  ${SRC}/app/ui/icon_button.cpp
  ${SRC}/app/ui/input_chain.cpp
  ${SRC}/app/ui/keyboard_shortcuts.cpp
  ${SRC}/app/ui/main_menu_bar.cpp
  ${SRC}/app/ui/main_window.cpp
  ${SRC}/app/ui/notifications.cpp
  ${SRC}/app/ui/palette_listbox.cpp
  ${SRC}/app/ui/palette_popup.cpp
  ${SRC}/app/ui/palette_view.cpp
  ${SRC}/app/ui/popup_window_pin.cpp
  ${SRC}/app/ui/preview_editor.cpp
  ${SRC}/app/ui/recent_listbox.cpp
  ${SRC}/app/ui/search_entry.cpp
  ${SRC}/app/ui/select_accelerator.cpp
  ${SRC}/app/ui/skin/button_icon_impl.cpp
  ${SRC}/app/ui/skin/skin_part.cpp
  ${SRC}/app/ui/skin/skin_property.cpp
  ${SRC}/app/ui/skin/skin_slider_property.cpp
  ${SRC}/app/ui/skin/skin_style_property.cpp
  ${SRC}/app/ui/skin/skin_theme.cpp
  ${SRC}/app/ui/skin/style.cpp
  ${SRC}/app/ui/skin/style_sheet.cpp
  ${SRC}/app/ui/status_bar.cpp
  ${SRC}/app/ui/styled_button.cpp
  ${SRC}/app/ui/tabs.cpp
  ${SRC}/app/ui/timeline.cpp
  ${SRC}/app/ui/toolbar.cpp
  ${SRC}/app/ui/touch_bar.cpp
  ${SRC}/app/ui/user_data_popup.cpp
  ${SRC}/app/ui/workspace.cpp
  ${SRC}/app/ui/workspace_panel.cpp
  ${SRC}/app/ui/workspace_tabs.cpp
  ${SRC}/app/ui/zoom_entry.cpp
  ${SRC}/app/ui_context.cpp
  ${SRC}/app/util/autocrop.cpp
  ${SRC}/app/util/clipboard.cpp
  ${SRC}/app/util/clipboard_native.cpp
  ${SRC}/app/util/create_cel_copy.cpp
  ${SRC}/app/util/expand_cel_canvas.cpp
  ${SRC}/app/util/filetoks.cpp
  ${SRC}/app/util/freetype_utils.cpp
  ${SRC}/app/util/msk_file.cpp
  ${SRC}/app/util/new_image_from_mask.cpp
  ${SRC}/app/util/pic_file.cpp
  ${SRC}/app/util/range_utils.cpp
  ${SRC}/app/widget_loader.cpp
  ${SRC}/app/xml_document.cpp
  ${SRC}/base/base64.cpp
  ${SRC}/base/cfile.cpp
  ${SRC}/base/chrono.cpp
  ${SRC}/base/convert_to.cpp
  ${SRC}/base/debug.cpp
  ${SRC}/base/dll.cpp
  ${SRC}/base/errno_string.cpp
  ${SRC}/base/exception.cpp
  ${SRC}/base/file_handle.cpp
  ${SRC}/base/fs.cpp
  ${SRC}/base/launcher.cpp
  ${SRC}/base/log.cpp
  ${SRC}/base/mem_utils.cpp
  ${SRC}/base/memory.cpp
  ${SRC}/base/memory_dump.cpp
  ${SRC}/base/mutex.cpp
  ${SRC}/base/path.cpp
  ${SRC}/base/process.cpp
  ${SRC}/base/program_options.cpp
  ${SRC}/base/replace_string.cpp
  ${SRC}/base/serialization.cpp
  ${SRC}/base/sha1.cpp
  ${SRC}/base/sha1_rfc3174.c
  ${SRC}/base/split_string.cpp
  ${SRC}/base/string.cpp
  ${SRC}/base/system_console.cpp
  ${SRC}/base/thread.cpp
  ${SRC}/base/time.cpp
  ${SRC}/base/trim_string.cpp
  ${SRC}/base/version.cpp
  ${SRC}/cfg/cfg.cpp
  ${SRC}/clip/clip.cpp
  ${SRC}/clip/clip_none.cpp
  ${SRC}/clip/image.cpp
  ${SRC}/css/compound_style.cpp
  ${SRC}/css/query.cpp
  ${SRC}/css/rule.cpp
  ${SRC}/css/sheet.cpp
  ${SRC}/css/style.cpp
  ${SRC}/css/value.cpp
  ${SRC}/doc/algo.cpp
  ${SRC}/doc/algorithm/flip_image.cpp
  ${SRC}/doc/algorithm/floodfill.cpp
  ${SRC}/doc/algorithm/polygon.cpp
  ${SRC}/doc/algorithm/resize_image.cpp
  ${SRC}/doc/algorithm/rotate.cpp
  ${SRC}/doc/algorithm/rotsprite.cpp
  ${SRC}/doc/algorithm/shift_image.cpp
  ${SRC}/doc/algorithm/shrink_bounds.cpp
  ${SRC}/doc/anidir.cpp
  ${SRC}/doc/blend_funcs.cpp
  ${SRC}/doc/blend_mode.cpp
  ${SRC}/doc/brush.cpp
  ${SRC}/doc/brush_type.cpp
  ${SRC}/doc/cel.cpp
  ${SRC}/doc/cel_data.cpp
  ${SRC}/doc/cel_data_io.cpp
  ${SRC}/doc/cel_io.cpp
  ${SRC}/doc/cels_range.cpp
  ${SRC}/doc/compressed_image.cpp
  ${SRC}/doc/context.cpp
  ${SRC}/doc/conversion_she.cpp
  ${SRC}/doc/document.cpp
  ${SRC}/doc/documents.cpp
  ${SRC}/doc/file/col_file.cpp
  ${SRC}/doc/file/gpl_file.cpp
  ${SRC}/doc/file/pal_file.cpp
  ${SRC}/doc/frame_tag.cpp
  ${SRC}/doc/frame_tag_io.cpp
  ${SRC}/doc/frame_tags.cpp
  ${SRC}/doc/handle_anidir.cpp
  ${SRC}/doc/image.cpp
  ${SRC}/doc/image_impl.cpp
  ${SRC}/doc/image_io.cpp
  ${SRC}/doc/images_collector.cpp
  ${SRC}/doc/layer.cpp
  ${SRC}/doc/layer_index.cpp
  ${SRC}/doc/layer_io.cpp
  ${SRC}/doc/layers_range.cpp
  ${SRC}/doc/mask.cpp
  ${SRC}/doc/mask_boundaries.cpp
  ${SRC}/doc/mask_io.cpp
  ${SRC}/doc/object.cpp
  ${SRC}/doc/palette.cpp
  ${SRC}/doc/palette_io.cpp
  ${SRC}/doc/primitives.cpp
  ${SRC}/doc/remap.cpp
  ${SRC}/doc/rgbmap.cpp
  ${SRC}/doc/site.cpp
  ${SRC}/doc/sort_palette.cpp
  ${SRC}/doc/sprite.cpp
  ${SRC}/doc/sprites.cpp
  ${SRC}/doc/string_io.cpp
  ${SRC}/doc/subobjects_io.cpp
  ${SRC}/doc/user_data_io.cpp
  ${SRC}/filters/color_curve.cpp
  ${SRC}/filters/color_curve_filter.cpp
  ${SRC}/filters/convolution_matrix.cpp
  ${SRC}/filters/convolution_matrix_filter.cpp
  ${SRC}/filters/invert_color_filter.cpp
  ${SRC}/filters/median_filter.cpp
  ${SRC}/filters/replace_color_filter.cpp
  ${SRC}/fixmath/fixmath.cpp
  ${SRC}/flic/decoder.cpp
  ${SRC}/flic/encoder.cpp
  ${SRC}/flic/stdio.cpp
  ${SRC}/gfx/clip.cpp
  ${SRC}/gfx/hsv.cpp
  ${SRC}/gfx/packing_rects.cpp
  ${SRC}/gfx/region.cpp
  ${SRC}/gfx/rgb.cpp
  ${SRC}/main/main.cpp
  ${SRC}/net/http_request.cpp
  ${SRC}/net/http_request_wasm.cpp
  ${SRC}/net/http_response.cpp
  ${SRC}/observable/obs/connection.cpp
  ${SRC}/render/get_sprite_pixel.cpp
  ${SRC}/render/quantization.cpp
  ${SRC}/render/render.cpp
  ${SRC}/render/zoom.cpp
  ${SRC}/script/cout_delegate.cpp
  ${SRC}/script/duktape/engine.cpp
  ${SRC}/she/common/freetype_font.cpp
  ${SRC}/she/sdl2/sdl2_display.cpp
  ${SRC}/she/sdl2/sdl2_surface.cpp
  ${SRC}/she/sdl2/she.cpp
  ${SRC}/ui/accelerator.cpp
  ${SRC}/ui/alert.cpp
  ${SRC}/ui/box.cpp
  ${SRC}/ui/button.cpp
  ${SRC}/ui/combobox.cpp
  ${SRC}/ui/cursor.cpp
  ${SRC}/ui/custom_label.cpp
  ${SRC}/ui/entry.cpp
  ${SRC}/ui/graphics.cpp
  ${SRC}/ui/grid.cpp
  ${SRC}/ui/image_view.cpp
  ${SRC}/ui/int_entry.cpp
  ${SRC}/ui/intern.cpp
  ${SRC}/ui/label.cpp
  ${SRC}/ui/link_label.cpp
  ${SRC}/ui/listbox.cpp
  ${SRC}/ui/listitem.cpp
  ${SRC}/ui/manager.cpp
  ${SRC}/ui/menu.cpp
  ${SRC}/ui/message.cpp
  ${SRC}/ui/message_loop.cpp
  ${SRC}/ui/move_region.cpp
  ${SRC}/ui/overlay.cpp
  ${SRC}/ui/overlay_manager.cpp
  ${SRC}/ui/paint_event.cpp
  ${SRC}/ui/panel.cpp
  ${SRC}/ui/popup_window.cpp
  ${SRC}/ui/property.cpp
  ${SRC}/ui/register_message.cpp
  ${SRC}/ui/resize_event.cpp
  ${SRC}/ui/scroll_bar.cpp
  ${SRC}/ui/scroll_helper.cpp
  ${SRC}/ui/separator.cpp
  ${SRC}/ui/size_hint_event.cpp
  ${SRC}/ui/slider.cpp
  ${SRC}/ui/splitter.cpp
  ${SRC}/ui/system.cpp
  ${SRC}/ui/textbox.cpp
  ${SRC}/ui/theme.cpp
  ${SRC}/ui/timer.cpp
  ${SRC}/ui/tooltips.cpp
  ${SRC}/ui/view.cpp
  ${SRC}/ui/viewport.cpp
  ${SRC}/ui/widget.cpp
  ${SRC}/ui/window.cpp
  ${SRC}/undo/undo_history.cpp
)

add_executable(libresprite ${SRC_FILES})

target_link_libraries( libresprite
                        SDL2
                        SDL2_image
                        SDL2_ttf
                        tinyxml2
                        pixman
                        modp_b64
                        simpleini
                      )
