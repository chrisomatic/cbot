#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <time.h>
#include <Objbase.h>
#include "resource.h"
#include "cfont.h"
#include "graphics.h"
#include "cstr.h"

#define MAX_NUM_ARGVS 50
#define MAX_TASKS 100
#define MAX_LOGS 1024 
#define BUFSIZE 4096
#define MAX_OUTPUT_LINES 32
#define MAX_ERROR_LINES 32

// define CBot colors
#define C_GREEN  0x00c0f0c0
#define C_YELLOW 0x00cfc267
#define C_RED    0x00be2c2f
#define C_GREY   0x006f6f6f
#define C_WHITE  0x00f0f0f0

// useful time definitions
#define SEC_PER_YEAR   31536000
#define SEC_PER_MONTH  2592000
#define SEC_PER_DAY    86400
#define SEC_PER_HOUR   360
#define SEC_PER_MINUTE 60

const char* CLASS_NAME = "cbot";

GUID guid;
HRESULT hCreateGuid = CoCreateGuid(&guid);

WNDCLASSEX wc;
HDC dc;
UINT WM_TASKBAR = 0;
HWND main_window;
HINSTANCE hInstance;
HMENU hmenu;
HCURSOR mcursor;
time_t curr_t;

char cbot_startup_message[] = "CBOT... \nBEEP BOOP!";
char* config_file;

typedef struct
{
	u8 r;
	u8 g;
	u8 b;
} CColor;

typedef enum
{
	TIME
	,EVENT
} CTaskType;

typedef struct
{
	char format_string[100];
	CTaskType type;
	char script_file[100];
    char logging_enabled;
	char remaining_time[14];
    long diff;
} CTask;

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    char year_str[5];
    char month_str[3];
	char day_str[3];
	char hour_str[3];
	char minute_str[3];
	char second_str[3];

    char time_display[25];

} CTime;

CTime current_time = {0};

typedef struct
{
    char scripts_folder[100];
	char logs_folder[100];
	char python_path[100];
	char r_path[100];
    int logging;
	int notifications;
    int pulse_color;
} Config;

Config cbot_config;

typedef enum
{
    C_MESSAGE,
    C_SYSTEM,
    C_WARNING,
    C_ERROR
} CLogType;

typedef struct
{
    CLogType log_type;
    char* text;
} CLog;

CLog logs[MAX_LOGS] = {};

CTask tasks[MAX_TASKS];

CFont font48;
CFont font22;
CFont font16;

int task_count = 0;

CColor title_color;
u32 title_color_u32;

CColor background_color;
CColor foreground_color;

BOOL is_running = TRUE;
BOOL is_paused = FALSE;
BOOL window_hidden = FALSE;
BOOL adding_new_log = FALSE;
BOOL on_divider = FALSE;
BOOL holding_divider = FALSE;

float divider_ratio = 0.5;
u32 bg_color;

int buffer_width;
int buffer_height;

int window_width;
int window_height;

int divider_x;
int prev_second;
int curr_second;

int process_output_line_count = 0;
int process_error_line_count = 0;

int logs_minimum_display_index = 0;
int log_line_height = 16;
int curr_log_line = 0;

int log_start_y = 26;
float log_new_inc = 0.0f;

char console_output[MAX_OUTPUT_LINES][BUFSIZE] = {};
char console_error[MAX_OUTPUT_LINES][BUFSIZE]  = {};

u8 brightness = 0xFF;
BOOL increase_brightness = FALSE;

HANDLE child_stdout = NULL;
HANDLE child_stderr = NULL;

HANDLE child_stdout_r = NULL;
HANDLE child_stdout_w = NULL;
HANDLE child_stderr_r = NULL;
HANDLE child_stderr_w = NULL;

BITMAPINFO bmi = {0};

static void minimize();
static void restore();
static char* get_tooltip(void);
static void update_cbot_tooltip(char* tooltip);
static void c_log(CLogType,char*);
static void draw_ui();
static void handle_tasks();
static void update_time_string(CTime* _time);
static void update_current_time();
static long get_time_difference(CTime* target_time, CTime* compare_time);
static void update_tasks();
static void load_config();
static BOOL run_process(char* script);
static void sort_task_list();
static void read_process_output(void);
static void read_process_error(void);

static BOOL icon_add();
static BOOL icon_delete();
static BOOL icon_message(char* title, char* message);
static BOOL icon_error(char* title, char* message);

static CTime get_test_time(char* format_string, CTime base_time);
static void handle_commandline(LPSTR cmdline);
static BOOL create_and_setup_window();

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

int argc = 0;
char *argv[MAX_NUM_ARGVS];

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, s32 nshowcmd)
{
    hInstance = hinstance;

    if(!create_and_setup_window())
        return 1;

	if (!icon_add())
		printf("Error adding icon!");
    
    handle_commandline(lpcmdline);

	if (argc > 0)
	{
		config_file = argv[0];
		update_tasks();

		char* tip = get_tooltip();
		update_cbot_tooltip(tip);
	}

    load_config();

    // setup backbuffer
	ShowWindow(main_window, SW_SHOWDEFAULT);

	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = buffer_width;
	bmi.bmiHeader.biHeight = -buffer_height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	back_buffer = malloc(buffer_width*buffer_height*4);

	dc = GetDC(main_window);
	MSG msg;

	time(&curr_t);
	srand((u32)curr_t);

    // setup font
	const char* font_name = "DroidSansMono.ttf";

	init_font(&font48, font_name, 48.0f);
	init_font(&font22, font_name, 22.0f);
	init_font(&font16, font_name, 16.0f);

	bg_color = 0x000F0F0F;
	title_color;

    // set title pulse color
    if(cbot_config.pulse_color)
    {
        title_color.r = cbot_config.pulse_color >> 16;
        title_color.g = cbot_config.pulse_color >> 8;
        title_color.b = cbot_config.pulse_color >> 0;
    }
    else
    {
        title_color.r = rand() % 256;
        title_color.g = rand() % 256;
        title_color.b = rand() % 256;
    }

	update_current_time();
	c_log(C_SYSTEM, "CBot Initialized.");

	time(&curr_t);
	prev_second = gmtime(&curr_t)->tm_sec;
    
    // MAIN_LOOP
	while (is_running)
	{
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

        if(!is_paused)
        {
            time(&curr_t);
            curr_second = gmtime(&curr_t)->tm_sec;

            // only handle tasks once every second
            if(prev_second != curr_second && task_count > 0)
            {
				update_tasks();
                handle_tasks();
                prev_second = curr_second;
            }
        } 
        
        if (!window_hidden)
            draw_ui();

		Sleep(5);	// @NOTE: to prevent CPU spending to much time in this process
	}

	ReleaseDC(main_window, dc);

	return EXIT_SUCCESS;
}

static BOOL create_and_setup_window()
{
    // Create and setup  window
	buffer_width = 1920;
	buffer_height = 1080;

	window_width = 800;
	window_height = 600;

	divider_x = window_width / 2;

	WM_TASKBAR = RegisterWindowMessageA("TaskbarCreated");

	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);

	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wc.lpszMenuName = NULL;
	wc.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(0x0f,0x0f,0x0f)));
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	
	if (!RegisterClassExA(&wc))
		return FALSE;

	DWORD window_style_ex = 0;
	DWORD window_style = WS_OVERLAPPEDWINDOW;

	RECT r = { 0 };
	r.right = window_width;
	r.bottom = window_height;

	// To make the drawable space truly BUFFER_WIDTH & BUFFER_HEIGHT
	AdjustWindowRectEx(&r, window_style, 0,window_style_ex);

	// Get screen dimensions to startup window at center-screen
	RECT w = { 0 };
	GetClientRect(GetDesktopWindow(), &w);

	main_window = CreateWindowEx(
		window_style_ex
		, CLASS_NAME
		, "CBot - Automation of Tasks"
		, window_style
		, (w.right / 2) - (window_width / 2)
		, (w.bottom / 2) - (window_height / 2)
		,r.right - r.left
		,r.bottom - r.top
		,HWND_DESKTOP
		,NULL
		,hInstance
		,0
		);

	// Enable accepting of dragged files
	DragAcceptFiles(main_window, TRUE);

    return TRUE;
}

static void handle_commandline(LPSTR cmdline)
{
	//handle command line
	while (*cmdline && (argc < MAX_NUM_ARGVS))
	{
		while (*cmdline && ((*cmdline <= 32) || (*cmdline > 126)))
			++cmdline;

		if (*cmdline)
		{
			argv[argc] = cmdline;
			++argc;

			while (*cmdline && ((*cmdline > 32) && (*cmdline) <= 126))
				++cmdline;

			if (*cmdline)
			{
				*cmdline = 0;
				++cmdline;
			}
		}
	}
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if (umsg == WM_TASKBAR && !IsWindowVisible(main_window))
	{
		minimize();
		return 0;
	}

	switch (umsg)
	{
	case WM_MOUSEWHEEL:
	{

		int fwKeys = GET_KEYSTATE_WPARAM(wparam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wparam);

		if (zDelta < 0)
			logs_minimum_display_index++;
		else if (zDelta > 0 && logs_minimum_display_index > 0)
			logs_minimum_display_index--;

		break;
	}
	case WM_SIZE:
	{
		window_width = LOWORD(lparam);
		window_height = HIWORD(lparam);
		divider_x = (int)(window_width*divider_ratio);
		break;
	}
	case WM_CREATE:
		ShowWindow(main_window, SW_HIDE);
		hmenu = CreatePopupMenu();
		AppendMenu(hmenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

		break;
	case WM_SYSCOMMAND:
		switch (wparam & 0xFFF0)
		{
		case SC_CLOSE:
            minimize();
			//icon_message("Beep Beep", "Just so you know - I'm minimized over here, still running!");
			return 0;
		}
		break;
	case WM_DROPFILES:
	{
		HDROP hdrop = (HDROP)wparam;
		int   dropped_file_count = 0;
		dropped_file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

		if (dropped_file_count > 0)
		{
			int file_name_length = DragQueryFile(hdrop, 0, NULL, 0);
			config_file = (char*)calloc(file_name_length + 1, sizeof(char));
			DragQueryFile(hdrop, 0, config_file, file_name_length + 1);

            update_tasks();

            if(task_count > 0)
                c_log(C_MESSAGE,"Tasks Loaded.");
		}

		DragFinish(hdrop);
		break;
	}
	case WM_SYSICON:
	{
		switch (wparam)
		{
		case ID_TRAY_APP_ICON:
			SetForegroundWindow(main_window);
			break;
		}
		if (lparam == WM_LBUTTONUP)
			restore();
		else if (lparam == WM_RBUTTONDOWN)
		{
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(main_window);

			UINT clicked = TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);
			SendMessage(hwnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away.

			if (clicked == ID_TRAY_EXIT)
			{
				if (!icon_delete())
					printf("Failed to delete icon.");

				is_running = FALSE;
				PostQuitMessage(0);
			}
		}
	}
	break;

	case WM_LBUTTONDOWN:
	{
		POINT pt;
		pt.x = GET_X_LPARAM(lparam);
		pt.y = GET_Y_LPARAM(lparam);

		if (pt.x >= 100 && pt.x <= 110){
			if (pt.y >= 2 && pt.y <= 12) {
				title_color.r = rand() % 256;
				title_color.g = rand() % 256;
				title_color.b = rand() % 256;
			}
		}

		if (on_divider)
		{
			holding_divider = TRUE;
		}
	} break;
	case WM_LBUTTONUP:
	{
		holding_divider = FALSE;
	} break;
	case WM_MOUSEMOVE:
	{
		if (holding_divider)
		{
			int x = GET_X_LPARAM(lparam);
			if (x < window_width)
			{
				divider_ratio = (float)x / window_width;
				draw_vertical_bar(divider_x, bg_color);
				divider_x = (int)(window_width*divider_ratio);
				draw_vertical_bar(divider_x, C_GREY);

				StretchDIBits(dc, 0, 0, window_width, window_height, 0, 0, window_width, window_height, back_buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
			}
		}
		else
		{
			POINT pt;
			pt.x = GET_X_LPARAM(lparam);
			pt.y = GET_Y_LPARAM(lparam);

			if (pt.x >= divider_x - 2 && pt.x <= divider_x + 2)
			{
				on_divider = TRUE;
				mcursor = LoadCursor(NULL, IDC_SIZEWE);
			}
			else
			{
				on_divider = FALSE;
				mcursor = NULL;
			}
		}
		
			
	} break;
	case WM_SETCURSOR:
	{
		if (mcursor)
		{
			SetCursor(mcursor);
			return TRUE;
		}
		break;
	}
	case WM_CLOSE:
		minimize();
		return 0;
		break;
	case WM_DESTROY:
		is_running = FALSE;
		PostQuitMessage(0);
		break;
	case WM_KEYUP:
	{
		switch (wparam)
		{
		case 0x50: // P
			if (is_paused)
            {
                is_paused = FALSE;
                c_log(C_SYSTEM, "CBot Unpaused.");
            }
			else 
            {
                is_paused = TRUE;
                c_log(C_SYSTEM, "CBot Paused.");
            }
			break;
		}
		break;
	}

	default: 
		break;
	}

	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

static CTime get_test_time(char* format_string, CTime base_time)
{
	char formatted_string[100] = { 0 };
	strcpy(formatted_string, format_string);

	str_replace(formatted_string, 100, "yyyy", base_time.year_str,formatted_string);
	str_replace(formatted_string, 100, "MM", base_time.month_str, formatted_string);
	str_replace(formatted_string, 100, "dd", base_time.day_str, formatted_string);
	str_replace(formatted_string, 100, "hh", base_time.hour_str, formatted_string);
	str_replace(formatted_string, 100, "mm", base_time.minute_str, formatted_string);
	str_replace(formatted_string, 100, "ss", base_time.second_str, formatted_string);
    
    CTime test_time;

    char* j = formatted_string;

    char* p = test_time.year_str;
    int period_counter = 0;

    while(*j)
    {
        if (*j == '.' || *j == ':')
        {
            ++period_counter;
            *p = '\0';
            
            switch (period_counter)
            {
                case 0: p = test_time.year_str;   break;
                case 1: p = test_time.month_str;  break;
                case 2: p = test_time.day_str;    break;
                case 3: p = test_time.hour_str;   break;
                case 4: p = test_time.minute_str; break;
                case 5: p = test_time.second_str; break;
            }
            ++j;
        }
        else
            *p++ = *j++;
    }

    test_time.year   = atoi(test_time.year_str);
    test_time.month  = atoi(test_time.month_str);
    test_time.day    = atoi(test_time.day_str);
    test_time.hour   = atoi(test_time.hour_str);
    test_time.minute = atoi(test_time.minute_str);
    test_time.second = atoi(test_time.second_str);

    return test_time;
}

static void handle_tasks()
{
    update_current_time();
    
    for(int i = 0; i < task_count; ++i)
    {
        CTime test_time = get_test_time(tasks[i].format_string,current_time);
        
		long diff = 0; 
		diff = get_time_difference(&test_time,&current_time);
        
        CTime new_base_time = {0};

        new_base_time.year = current_time.year;
        new_base_time.month = current_time.month;
        new_base_time.day = current_time.day;
        new_base_time.hour = current_time.hour;
        new_base_time.minute = current_time.minute;
        new_base_time.second = current_time.second;
        
        int comp_counter = 0;

        while(diff < 0 && comp_counter < 6)
        {
            switch(comp_counter++)
            {
                case 0: new_base_time.second++; break;
                case 1: new_base_time.minute++; break;
                case 2: new_base_time.hour++;   break;
                case 3: new_base_time.day++;    break;
                case 4: new_base_time.month++;  break;
                case 5: new_base_time.year++;   break;
            }
            
            update_time_string(&new_base_time); 
            test_time = get_test_time(tasks[i].format_string,new_base_time);
            update_time_string(&test_time);

            diff = get_time_difference(&test_time, &current_time);
        }

        // update remaining time
        // format: 000d 00:00:00

        if(diff < 0)
        {
            C_strncpy(tasks[i].remaining_time,"---d --:--:--",13);
            tasks[i].diff = 2147483647; // max long int value
        }
        else
        {
            tasks[i].diff = diff;

            long temp_diff = diff;

            int years   = temp_diff / SEC_PER_YEAR;   temp_diff -= (SEC_PER_YEAR*years);
            int months  = temp_diff / SEC_PER_MONTH;  temp_diff -= (SEC_PER_MONTH*months);
            int days    = temp_diff / SEC_PER_DAY;    temp_diff -= (SEC_PER_DAY*days);
            int hours   = temp_diff / SEC_PER_HOUR;   temp_diff -= (SEC_PER_HOUR*hours);
            int minutes = temp_diff / SEC_PER_MINUTE; temp_diff -= (SEC_PER_MINUTE*minutes);
            int seconds = temp_diff;

            CTime temp = {0};
            
            temp.year   = years;
            temp.month  = months;
            temp.day    = days;
            temp.hour   = hours;
            temp.minute = minutes;
            temp.second = seconds;

            char day[4] = {0};
            int temp_days = temp.year*365 + temp.month*30 + temp.day;

            _itoa(temp_days, day, 10);
            _itoa(temp.hour,   temp.hour_str,   10);
            _itoa(temp.minute, temp.minute_str, 10);
            _itoa(temp.second, temp.second_str, 10);


            if (temp_days > 999)
            {
                day[0] = '9';
                day[1] = '9';
                day[2] = '9';
                day[3] = '\0';
            }
            else if (temp_days > 10 && temp_days < 100)
            {
                day[2] = day[0];
                day[1] = day[1];
                day[0] = '0'; 
                day[3] = '\0';

            }
            else if (temp_days < 10)
            {
                day[2] = day[0];
                day[0] = '0';
                day[1] = '0'; 
                day[3] = '\0';
            }

            if (temp.hour < 10)
            {
                temp.hour_str[1] = temp.hour_str[0];
                temp.hour_str[0] = '0';
                temp.hour_str[2] = '\0';
            }

            if (temp.minute < 10)
            {
                temp.minute_str[1] = temp.minute_str[0];
                temp.minute_str[0] = '0';
                temp.minute_str[2] = '\0';
            }
            
            if (temp.second < 10)
            {
                temp.second_str[1] = temp.second_str[0];
                temp.second_str[0] = '0';
                temp.second_str[2] = '\0';
            }

            tasks[i].remaining_time[0] = day[0]; 
            tasks[i].remaining_time[1] = day[1];
            tasks[i].remaining_time[2] = day[2];
            tasks[i].remaining_time[3] = 'd'; 
            tasks[i].remaining_time[4] = ' ';
            tasks[i].remaining_time[5] = temp.hour_str[0];
            tasks[i].remaining_time[6] = temp.hour_str[1];
            tasks[i].remaining_time[7] = ':';
            tasks[i].remaining_time[8] = temp.minute_str[0];
            tasks[i].remaining_time[9] = temp.minute_str[1];
            tasks[i].remaining_time[10] = ':';
            tasks[i].remaining_time[11] = temp.second_str[0];
            tasks[i].remaining_time[12] = temp.second_str[1];
            tasks[i].remaining_time[13] = '\0';
        }

        if(diff == 0)
        {
            // run task
            if(tasks[i].logging_enabled != '0')
                c_log(C_MESSAGE,C_concat(3,"Running ",tasks[i].script_file,"!"));

            if(run_process(tasks[i].script_file))
            {
				read_process_output();
				read_process_error();
            }
        }
    }
}

static void read_process_output(void)
{
	CHAR chBuf[BUFSIZE] = { 0 };
	DWORD bytes_read;
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    process_output_line_count = 0;

	bSuccess = ReadFile(child_stdout, chBuf, BUFSIZE, &bytes_read, NULL);
    strcpy(console_output[process_output_line_count++], chBuf); 

}

static void read_process_error(void)
{
	CHAR chBuf[BUFSIZE] = { 0 };
	DWORD bytes_read;
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    process_error_line_count = 0;

	bSuccess = ReadFile(child_stderr, chBuf, BUFSIZE, &bytes_read, NULL);
    strcpy(console_error[process_error_line_count++], chBuf);

}

static long get_time_difference(CTime* target_time, CTime* compare_time)
{
    // total difference in seconds
	long diff = 0; 

    diff += SEC_PER_YEAR   *(target_time->year   - compare_time->year);
    diff += SEC_PER_MONTH  *(target_time->month  - compare_time->month);
    diff += SEC_PER_DAY    *(target_time->day    - compare_time->day);
    diff += SEC_PER_HOUR   *(target_time->hour   - compare_time->hour);
    diff += SEC_PER_MINUTE *(target_time->minute - compare_time->minute);
    diff += target_time->second - compare_time->second;

    return diff;
}

static void update_current_time()
{
	time(&curr_t);

	current_time.year   = 1900 + localtime(&curr_t)->tm_year;
	current_time.month  = 1 + localtime(&curr_t)->tm_mon;
	current_time.day    = localtime(&curr_t)->tm_mday;
	current_time.hour   = localtime(&curr_t)->tm_hour;
	current_time.minute = localtime(&curr_t)->tm_min;
	current_time.second = localtime(&curr_t)->tm_sec;

    update_time_string(&current_time);
}

static void update_time_string(CTime* _time)
{
	_itoa(_time->year,   _time->year_str,   10);
	_itoa(_time->month,  _time->month_str,  10);
	_itoa(_time->day,    _time->day_str,    10);
	_itoa(_time->hour,   _time->hour_str,   10);
	_itoa(_time->minute, _time->minute_str, 10);
	_itoa(_time->second, _time->second_str, 10);

	if (_time->month < 10)
	{
		_time->month_str[1] = _time->month_str[0];
		_time->month_str[0] = '0';
		_time->month_str[2] = '\0';
	}

	if (_time->day < 10)
	{
		_time->day_str[1] = _time->day_str[0];
		_time->day_str[0] = '0';
		_time->day_str[2] = '\0';
	}

	if (_time->hour < 10)
	{
		_time->hour_str[1] = _time->hour_str[0];
		_time->hour_str[0] = '0';
		_time->hour_str[2] = '\0';
	}

	if (_time->minute < 10)
	{
		_time->minute_str[1] = _time->minute_str[0];
		_time->minute_str[0] = '0';
		_time->minute_str[2] = '\0';
	}
    
	if (_time->second < 10)
	{
		_time->second_str[1] = _time->second_str[0];
		_time->second_str[0] = '0';
		_time->second_str[2] = '\0';
	}

    char* time_string = C_concat(11, _time->year_str, "/", _time->month_str, "/", _time->day_str, " ", _time->hour_str, ".", _time->minute_str, ".", _time->second_str); 
    
	int a;
	if (task_count > 0)
		a = 5;
    int i_display = 0;

	char* time_string_p = time_string;
    while(*time_string_p)
    {
		_time->time_display[i_display] = *time_string_p;

        ++time_string_p;
        ++i_display;
    } 
    
    _time->time_display[i_display] = '\0';

	free(time_string);

}

static void sort_task_list()
{
    CTask temp = {0};

    for (int i = 0; i < task_count; ++i)
    {
        for(int j = i + 1; j < task_count; ++j)
        {
            if(tasks[i].diff > tasks[j].diff)
            {
                temp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = temp;
            }
        }
    }
}

static void draw_ui()
{
    memset(back_buffer, bg_color, buffer_width*buffer_height*4);

    if(!is_paused)
    {
        if (increase_brightness)
            ++brightness;
        else
            --brightness;

        if (brightness == 50)
            increase_brightness = TRUE;
        else if (brightness == 255)
            increase_brightness = FALSE;

        float brightness_ratio = brightness / 255.0f;

        u8 r = (u8)(title_color.r * brightness_ratio);
        u8 g = (u8)(title_color.g * brightness_ratio);
        u8 b = (u8)(title_color.b * brightness_ratio);

        title_color_u32 = (r << 16 | g << 8 | b);
    }
        
    draw_string("CBot", &font48, 2, 0, title_color_u32);
    draw_string("Drag and drop a .cbt file", &font16, 2, 48, C_GREY);
    draw_string("Press P to toggle pause", &font16, 2, 64, C_GREY);

    draw_horizontal_bar_with_bounds(88, 0, divider_x, C_GREY);
    draw_vertical_bar(divider_x, C_GREY);

    draw_string("Log", &font22, divider_x + 3, 2, C_WHITE);

    if (adding_new_log)
    {
        log_new_inc += 1.5f;
        ++log_start_y;

        if (log_new_inc >= 16)
        {
            log_new_inc = 0;
            log_start_y = 26;
            adding_new_log = FALSE;
        }
    }

    u32 log_color;

    int j = 0;
    int log_begin_index = curr_log_line - 1 - logs_minimum_display_index;

    for (int i = log_begin_index; i >= 0; --i)
    {
            switch(logs[i].log_type)
            {
                case C_MESSAGE: log_color = C_GREEN;  break;
                case C_SYSTEM:  log_color = C_GREY;   break;
                case C_WARNING: log_color = C_YELLOW; break;
                case C_ERROR:   log_color = C_RED;    break;
            }

            if (i < log_begin_index || !adding_new_log)
            {
                draw_string(logs[i].text, &font16, divider_x + 3, log_start_y + j * log_line_height, log_color);
                ++j;
            }

			if (log_start_y + (j + 1) * log_line_height > buffer_height)
				break;
    }

	// update scroll index text

	char log_index_min[4] = { 0 };
	char log_index_max[4] = { 0 };

	_itoa(logs_minimum_display_index + 1, log_index_min, 10);
	_itoa(curr_log_line, log_index_max, 10);

    char* log_index = C_concat(3,log_index_min,"-",log_index_max);
    draw_string(log_index, &font16,divider_x + 42,2,C_GREY);
	free(log_index);

    draw_string("Tasks", &font22, 2, 91, C_WHITE);

    // draw task table headers
    //draw_string("Remaining Time  Scripts", &font16,2,113,C_WHITE);

    sort_task_list();

    int curr_y = 114;
    for (int i = 0; i < task_count; ++i)
    {
        char* task_display = C_concat(3, tasks[i].remaining_time," | ",tasks[i].script_file);
        draw_string(task_display, &font16, 2, curr_y, 0x00808080);
        free(task_display);
        curr_y += 16;
    }

    // draw console area
    draw_horizontal_bar_with_bounds(window_height - 100, 0, divider_x, C_GREY);
    draw_string("Output",&font22,2,window_height - 99,C_WHITE);

	// draw most recent console output
    for(int i = 0; i < process_output_line_count;++i)
        draw_string(console_output[i], &font16, 2, window_height - 20, C_YELLOW);

	// draw most recent console error
    for(int i = 0; i < process_error_line_count;++i)
        draw_string(console_error[i], &font16, 2, window_height - 40, C_RED);

    draw_rect(100, 2, 110, 12,0x004F4F4F);

    if(is_paused)
        draw_string("PAUSED", &font22, window_width - 68, window_height - 22, C_GREY);

    StretchDIBits(dc, 0, 0, window_width, window_height, 0, 0, window_width, window_height, back_buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);

}

static void update_tasks()
{
    if (!config_file)
		return;

    FILE* fp;
    char buffer[1024] = { 0 };

    fp = fopen(config_file, "r");
	if (fp == NULL)
		return;
    
    task_count = 0;
    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (buffer[0] == '@') // Time-based task
        {
            CTask newtask = { 0 };
			char  format_string[100] = { 0 };
			char  script_file[100] = { 0 };
            char  logging_enabled = 0;

            int iField = 0;
            int format_string_length = 0;
            int script_length = 0;

            // Parse the each line
            for (int i = 1; i < sizeof(buffer); ++i)
            {
                if (buffer[i] == '\0' || buffer[i] == '#') break;
                if (buffer[i] == ';')
                {
                    ++iField;
                    if (i + 1 < sizeof(buffer))
                        ++i;
                }
                if (buffer[i] >= 33 && buffer[i] <= 126)
                {
                    switch (iField)
                    {
                    case 0: format_string[format_string_length++] = buffer[i]; break;
                    case 1: script_file[script_length++] = buffer[i]; break;
                    case 2: logging_enabled = buffer[i]; break;
                    }

                }
            }

            newtask.type = TIME;
			strcpy(newtask.format_string,format_string);
			strcpy(newtask.script_file, script_file);

            newtask.logging_enabled = logging_enabled;
            tasks[task_count++] = newtask;

        }
    }

	fclose(fp);
}
static void load_config()
{
    FILE *fp = fopen("cbot.config", "r");

	// clear out config values
	memset(cbot_config.scripts_folder,0,100 * sizeof(char));
	memset(cbot_config.logs_folder, 0, 100 * sizeof(char));
	memset(cbot_config.python_path, 0, 100 * sizeof(char));
	memset(cbot_config.r_path, 0, 100 * sizeof(char));

	cbot_config.logging = FALSE;
    cbot_config.pulse_color = 0;

	// expected:
	// key : value \n
	// key : value \n
	// ...
	// key : value eof

	BOOL is_key = TRUE;

	int ikey = 0;
	int ivalue = 0;

	char key[100] = { 0 };
	char value[100] = { 0 };

	int c;

	do
	{
		c = fgetc(fp);
		if (c == '=')
		{
			is_key = FALSE;
			key[ikey] = '\0';
			ikey = 0;
		}
		else if (c == '\n' || c == EOF)
		{
			is_key = TRUE;
			ivalue = 0;
		
			if      (strcmp(key, "SCRIPTS_FOLDER") == 0) strcpy(cbot_config.scripts_folder,value);
			else if (strcmp(key, "LOGS_FOLDER")    == 0) strcpy(cbot_config.logs_folder,value);
			else if (strcmp(key, "PYTHON_PATH")    == 0) strcpy(cbot_config.python_path,value);
			else if (strcmp(key, "R_PATH")         == 0) strcpy(cbot_config.r_path,value);
            else if (strcmp(key, "LOGGING")        == 0) C_atoi(value, &cbot_config.logging);
            else if (strcmp(key, "NOTIFICATIONS")  == 0) C_atoi(value, &cbot_config.notifications);
            else if (strcmp(key, "PULSE_COLOR")    == 0) C_atoi(value, &cbot_config.pulse_color);

			memset(key, 0, 100);
			memset(value, 0, 100);
		}
		else
		{
			if (is_key)
				key[ikey++] = c;
			else
				value[ivalue++] = c;
		}

	} while (c != EOF);

	fclose(fp);
}

static BOOL run_process(char* script)
{
    //char* app_name;

    BOOL is_python = (str_contains(script,".py") || str_contains(script,".Py") || str_contains(script,".PY"));
    BOOL is_r      = (str_contains(script,".r")  || str_contains(script,".R"));

    char* command = "";

    if(is_python)
    {
		command = C_concat(6, cbot_config.python_path, " \"", cbot_config.scripts_folder, "\\", script, "\"");
    }
	else if (is_r)
    {   
		command = C_concat(6, cbot_config.r_path, " \"", cbot_config.scripts_folder, "\\", script, "\"");
    }
	else
    {
		command = C_concat(7,"cmd.exe ","/c ", "\"", cbot_config.scripts_folder, "\\", script, "\"");
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

	child_stdout = CreateFile(
		".\\tempout.txt",
		GENERIC_READ | GENERIC_WRITE,
		0,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	child_stderr = CreateFile(
		".\\temperr.txt",
		GENERIC_READ | GENERIC_WRITE,
		0,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);
	/*
    if( !CreatePipe(&child_stdout_r,&child_stdout_w,&sa,0))
    {
       icon_error("Error","Failed to create child pipe!");
       return FALSE; 
    }

    if( !CreatePipe(&child_stderr_r,&child_stderr_w,&sa,0))
    {
       icon_error("Error","Failed to create child pipe!");
       return FALSE; 
    }

    if( !SetHandleInformation(child_stdout_r, HANDLE_FLAG_INHERIT,0))
    {
       icon_error("Error","Read Handle for pipe is inherited!");
       return FALSE;
    }

    if( !SetHandleInformation(child_stderr_r, HANDLE_FLAG_INHERIT,0))
    {
       icon_error("Error","Read Handle for pipe is inherited!");
       return FALSE;
    }
	*/

	if (!SetHandleInformation(child_stdout, HANDLE_FLAG_INHERIT, 0))
	{
		icon_error("Error", "Read Handle for file is inherited!");
		return FALSE;
	}

	if (!SetHandleInformation(child_stderr, HANDLE_FLAG_INHERIT, 0))
	{
		icon_error("Error", "Read Handle for file is inherited!");
		return FALSE;
	}

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory (&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory (&si, sizeof(STARTUPINFO));

    si.cb = sizeof(STARTUPINFO);
	si.hStdError = child_stderr;//child_stderr_w;
	si.hStdOutput = child_stdout;//child_stdout_w;
    si.hStdInput = NULL;
	si.dwFlags |= STARTF_USESTDHANDLES;

	BOOL result = CreateProcess(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	if (!result)
	{
		char* error_message = C_concat(2, "Error running ", command);
		c_log(C_ERROR, error_message);

		if(window_hidden && cbot_config.notifications)
			icon_error("Error", error_message);
        
        free(error_message);
    }
    else
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

	free(command);

    return result;
}

static void c_log(CLogType log_type,char* m)
{
    adding_new_log = TRUE;

    CLog new_log;

    new_log.log_type = log_type;
    new_log.text = C_concat(3, current_time.time_display, ": ", m);

    logs[curr_log_line++] = new_log;
}

static char* get_tooltip(void)
{
	char* tip;
	char tasks[4] = { 0 };
	_itoa(task_count, tasks, 10);

	tip = C_concat(2, tasks, " Tasks Loaded.");

	return tip;
}

static void update_cbot_tooltip(char* tooltip)
{
	NOTIFYICONDATA nid = { 0 };

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = guid;

	memset(nid.szTip, 0, 128);

	C_strncpy(nid.szTip, tooltip, strlen(tooltip));
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static BOOL icon_add()
{
    NOTIFYICONDATA nid = {0};

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = main_window;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = guid;
	nid.uCallbackMessage = WM_SYSICON; //Set up our invented Windows Message 
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	C_strncpy(nid.szTip, cbot_startup_message, sizeof(cbot_startup_message));
	nid.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_ADD, &nid);

	return  Shell_NotifyIcon(NIM_SETVERSION, &nid); 
}

static BOOL icon_delete()
{
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_GUID;
	nid.guidItem = guid;

	return Shell_NotifyIcon(NIM_DELETE, &nid);
}
static BOOL icon_message(char* title, char* message)
{
    NOTIFYICONDATA nid = {};

    nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_INFO | NIF_GUID;
	nid.guidItem = guid;
	nid.uTimeout = 200;

	// respect quiet time since this balloon did not come from a direct user action.
	nid.dwInfoFlags = NIIF_INFO | NIIF_RESPECT_QUIET_TIME;

	C_strncpy(nid.szInfoTitle, title, 64);
	C_strncpy(nid.szInfo, message, 256);

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static BOOL icon_error(char* title, char* message)
{
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_INFO | NIF_GUID;
	nid.guidItem = guid;
	nid.dwInfoFlags = NIIF_ERROR;
	nid.uTimeout = 200;

	C_strncpy(nid.szInfoTitle, title, 64);
	C_strncpy(nid.szInfo, message, 256);

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static void minimize()
{
	ShowWindow(main_window, SW_HIDE);
	window_hidden = TRUE;
}

static void restore()
{
	ShowWindow(main_window, SW_SHOW);
	window_hidden = FALSE;
}
