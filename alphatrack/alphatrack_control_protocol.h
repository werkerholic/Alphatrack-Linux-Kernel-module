/*
  Copyright (C) 2006 Paul Davis 
  Copyright (C) 2007 Mike Taht

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#ifndef ardour_alphatrack_control_protocol_h
#define ardour_alphatrack_control_protocol_h

#include "alphatrack_base.h"

#include <vector>
#include <bitset>
#include <sys/time.h>
#include <pthread.h>
#include <glibmm/thread.h>
#include <ardour/types.h>

#include <control_protocol/control_protocol.h>

class AlphatrackControlProtocol : public ARDOUR::ControlProtocol
{
public:
	AlphatrackControlProtocol (ARDOUR::Session&);
	virtual ~AlphatrackControlProtocol();

	int set_active (bool yn);

	static bool probe ();

	XMLNode& get_state ();
	int set_state (const XMLNode&);

private:
	const static int STATUS_OFFLINE  = 0xff;
	const static int STATUS_ONLINE = 0x01;
	const static int STATUS_OK = 0x00;
  
        const static int LIGHTS = 25;
        const static int ROWS = 2;
        const static int COLUMNS = 16;

	enum LightID {
	  LightEQ = 0,
	  LightOut,
	  LightF2,
	  LightSend,
	  LightIn,
	  LightF1,
	  LightPan,
	  LightUNDEF1,
	  LightUNDEF2,
	  LightShift,
	  LightTrackmute,
	  LightTracksolo,
	  LightTrackrec,
	  LightRead,
	  LightWrite,
	  LightAnysolo,
	  LightAuto,
	  LightF4,
	  LightRecord,
	  LightWindow,
	  LightPlugin,
	  LightF3,
	  LightLoop
	};

	enum ButtonID {
	  ButtonBattery     =0x00004000,
	  ButtonBacklight   =0x00008000,
	  ButtonFastForward =0x04000000,
	  ButtonTrackMute   =0x00040000,
	  ButtonTrackSolo   =0x00800000,
	  ButtonTrackLeft   =0x80000000,
	  ButtonRecord      =0x02000000,
	  ButtonShift       =0x20000000,
	  ButtonPunch       =0x00800000,
	  ButtonTrackRight  =0x00020000,
	  ButtonRewind      =0x01000000,
	  ButtonStop        =0x10000000,
	  ButtonLoop        =0x00010000,
	  ButtonTrackRec    =0x00001000,
	  ButtonPlay        =0x08000000,
	  ButtonTouch1      =0x00000008,
	  ButtonTouch2      =0x00000010,
	  ButtonTouch3      =0x00000020,
	  ButtonPress1      =0x00000009,
	  ButtonPress2      =0x00008010,
	  ButtonPress3      =0x00002020,

// last 3 bytes are the slider position
// 40 is the actual slider moving, the most sig bits, and 3 lsb

	  ButtonFlip         =0x40000000,
	  ButtonF1           =0x00100000,
	  ButtonF2           =0x00400000,
	  ButtonF3           =0x00200000,
	  ButtonF4           =0x00080000,
	  ButtonPan          =0x00000200,
	  ButtonSend         =0x00000800,
	  ButtonEQ           =0x00004000,
	  ButtonPlugin       =0x00000400,
	  ButtonAuto         =0x00000100,
	  ButtonFootswitch = 0x11111111, // FIXME, lie
	};

	enum WheelShiftMode {
		WheelShiftGain,
		WheelShiftPan,
		WheelShiftMaster,
		WheelShiftMarker
	};
		
	enum WheelMode {
		WheelTimeline,
		WheelScrub,
		WheelShuttle
	};

	// FIXME - look at gtk2_ardour for snap settings

	enum WheelIncrement {
		WheelIncrSlave,
		WheelIncrScreen,
		WheelIncrSample,
		WheelIncrBeat,
		WheelIncrBar,
		WheelIncrSecond,
		WheelIncrMinute
	};
	  
	enum DisplayMode {
		DisplayNormal,
		DisplayRecording,
		DisplayRecordingMeter,
		DisplayBigMeter,
		DisplayConfig,
		DisplayBling,
		DisplayBlingMeter
	};

	enum BlingMode {
	        BlingOff,
	        BlingKit,
	        BlingRotating,
	        BlingPairs,
	        BlingRows,
	        BlingFlashAll,
		BlingEnter,
		BlingExit
	};
	
	pthread_t       thread;
	int udev;

#if ALPHATRACK_THREADS
	pthread_t       thread_read;
#endif
	int             last_read_error;

	uint64_t        buttonmask;
	uint32_t        timeout;
	uint32_t        inflight;
	uint32_t        current_track_id;
#if ALPHATRACK_THREADS
	pthread_t       thread_write;
#endif
	int             last_write_error;
	uint8_t        _datawheel;
	uint8_t        _device_status;
	WheelMode       wheel_mode;
	WheelShiftMode  wheel_shift_mode;
	DisplayMode     display_mode;
	BlingMode       bling_mode;
	WheelIncrement  wheel_increment;

	ARDOUR::gain_t  gain_fraction;

	Glib::Mutex update_lock;

        std::bitset<ROWS*COLUMNS> screen_invalid;
	char screen_current[ROWS][COLUMNS];
	char screen_pending[ROWS][COLUMNS];

        std::bitset<LIGHTS> lights_invalid;
        std::bitset<LIGHTS> lights_current;
        std::bitset<LIGHTS> lights_pending;
        std::bitset<LIGHTS> lights_flash;

	int32_t        last_notify;
	char           last_notify_msg[COLUMNS+1]; 
	uint32_t       last_bars;
	uint32_t       last_beats;
	uint32_t       last_ticks;

	bool           last_negative;
	uint32_t       last_hrs;
	uint32_t       last_mins;
	uint32_t       last_secs;
	uint32_t       last_frames;
	nframes_t      last_where;
	ARDOUR::gain_t last_track_gain;
	uint32_t       last_meter_fill;
	struct timeval last_wheel_motion;
	int            last_wheel_dir;

	Glib::Mutex io_lock;

	int open ();
	int read (uint8_t *buf,uint32_t timeout_override = 0);
	int write (uint8_t* cmd, uint32_t timeout_override = 0);
	int write_noretry (uint8_t* cmd, uint32_t timeout_override = 0);
	int close ();
	int save_config(char *name = "default");
	int load_config(char *name = "default");
	int save(char *name);
	int load(char *name);
        void print (int row, int col, const char* text);
	void print_noretry (int row, int col, const char* text);
	void notify(const char *msg);
	static void* _monitor_work (void* arg);
	void* monitor_work ();

	int process (uint8_t *);
	int update_state();
	void invalidate();
	int flush();
	// A screen is a cache of what should be on the lcd

	void screen_init();
	void screen_validate();
	void screen_invalidate();
	int  screen_flush();
	void screen_clear();
	// bool screen_isuptodate(); // think on this - 
	int  screen_show_bling();

	// Commands to write to the lcd 

	int  lcd_init();
        bool lcd_damage();
	bool lcd_isdamaged();

        bool lcd_damage(int row, int col = 0, int length = COLUMNS);
	bool lcd_isdamaged(int row, int col = 0, int length = COLUMNS);

	int  lcd_flush();
	int  lcd_write(uint8_t* cmd, uint32_t timeout_override = 0); // pedantic alias for write
	void lcd_fill (uint8_t fill_char);
	void lcd_clear ();
	void lcd_print (int row, int col, const char* text);
	void lcd_print_noretry (int row, int col, const char* text);

	// Commands to write to the lights
	// FIXME - on some devices lights can have intensity and colors

	void lights_init();
	void lights_validate();
	void lights_invalidate();
	void light_validate(LightID light);
	void light_invalidate(LightID light);
	int  lights_flush();
	int  lights_write(uint8_t* cmd,uint32_t timeout_override = 0); // pedantic alias to write

	// a cache of what should be lit

	void lights_off ();
	void lights_on ();
	int  light_set(LightID, bool offon = true);
	int  light_on (LightID);
	int  light_off (LightID);

	// some modes for the lights, should probably be renamed

	int  lights_show_normal();
	int  lights_show_recording();
	int  lights_show_tempo();
	int  lights_show_bling();

	void enter_big_meter_mode ();
	void enter_normal_display_mode ();
	void enter_config_mode();
	void enter_recording_mode();
	void enter_bling_mode();

	void next_marker (); // basicui doesn't give me enough info
	void prev_marker (); 

	void next_display_mode ();
	void normal_update ();

	void show_current_track ();
	void show_track_gain ();
	void show_transport_time ();
	void show_bbt (nframes_t where);	
	void show_smpte (nframes_t where);
	void show_wheel_mode ();
	void show_gain ();
	void show_pan ();
	void show_meter ();
	void show_mini_meter ();
	void show_bling();
	void show_notify();

	//	void datawheel ();
	// void scrub ();
	// void scroll ();
	// void shuttle ();
	void config ();

	void next_wheel_mode ();
	void next_wheel_shift_mode ();

	void set_current_track (ARDOUR::Route*);
	void next_track ();
	void prev_track ();
	void step_gain_up ();
	void step_gain_down ();
	void step_pan_right ();
	void step_pan_left ();

	//	void button_event_backlight_press (bool shifted);
	// void button_event_backlight_release (bool shifted);
	void button_event_trackleft_press (bool shifted);
	void button_event_trackleft_release (bool shifted);
	void button_event_trackright_press (bool shifted);
	void button_event_trackright_release (bool shifted);
	void button_event_trackrec_press (bool shifted);
	void button_event_trackrec_release (bool shifted);
	void button_event_trackmute_press (bool shifted);
	void button_event_trackmute_release (bool shifted);
	void button_event_tracksolo_press (bool shifted);
	void button_event_tracksolo_release (bool shifted);
	void button_event_undo_press (bool shifted);
	void button_event_undo_release (bool shifted);
	void button_event_in_press (bool shifted);
	void button_event_in_release (bool shifted);
	void button_event_out_press (bool shifted);
	void button_event_out_release (bool shifted);
	void button_event_punch_press (bool shifted);
	void button_event_punch_release (bool shifted);
	void button_event_loop_press (bool shifted);
	void button_event_loop_release (bool shifted);
	void button_event_prev_press (bool shifted);
	void button_event_prev_release (bool shifted);
	void button_event_add_press (bool shifted);
	void button_event_add_release (bool shifted);
	void button_event_next_press (bool shifted);
	void button_event_next_release (bool shifted);
	void button_event_rewind_press (bool shifted);
	void button_event_rewind_release (bool shifted);
	void button_event_fastforward_press (bool shifted);
	void button_event_fastforward_release (bool shifted);
	void button_event_stop_press (bool shifted);
	void button_event_stop_release (bool shifted);
	void button_event_play_press (bool shifted);
	void button_event_play_release (bool shifted);
	void button_event_record_press (bool shifted);
	void button_event_record_release (bool shifted);
	void button_event_footswitch_press(bool shifted);
	void button_event_footswitch_release (bool shifted);

};


#endif // ardour_alphatrack_control_protocol_h
