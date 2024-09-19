#pragma once

extern void update_line_status();

extern void main_widget();

extern uint32_t BL_EVENT_1;
#if ESP_PLATFORM
#else
extern uint32_t MY_EVENT_1;
#endif
