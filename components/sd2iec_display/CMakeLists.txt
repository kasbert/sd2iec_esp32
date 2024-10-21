idf_component_register(SRCS
        "lv_file_explorer.c"
        "ui.c"
        "ui_main.c"
        "ui_config.c"
        "ui_status.c"
        "ui_files.c"
        "ui_about.c"
        "hw.c"
                INCLUDE_DIRS "."
                REQUIRES sd2iec esp_lvgl_port)
