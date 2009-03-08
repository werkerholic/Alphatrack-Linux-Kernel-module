/* We need some limited communication with the editor for the
   snap to settings */

#include <tranzport_base.h>
#include <tranzport_common.h>
#include <tranzport_control_protocol.h>
#include <ardour/location.h>

#include "i18n.h"

using namespace Editing;

static inline void FUDGE_64BIT_INC(nframes64_t &a, int dir) {
  switch(dir) {
  case 0: break;
  case 1: a = MIN(++a,UINT_MAX); 
  case -1: a = ZEROIFNEG(--a);
  default: break;
  }
}

XMLNode *TranzportControlProtocol::editor_settings ()
{
        XMLNode* node = 0;

        if (session) {
                node = session->instant_xml(X_("Editor"), session->path());
//        } else {
//             node = Config->instant_xml(X_("Editor"), get_user_ardour_path());
        }
        return node;
}


Editing::SnapType TranzportControlProtocol::get_snapto () 
{
	const XMLProperty* prop;
	XMLNode *node = 0;
	if ((node = editor_settings()) == 0) {
	  snap_to = Editing::SnapToSMPTESeconds;
	  snap_mode = Editing::SnapNormal; // Grid
	  return snap_to;
	}

        if ((prop = node->property ("snap-to"))) {
	  snap_to = (Editing::SnapType) atoi (prop->value().c_str());
        }

        if ((prop = node->property ("snap-mode"))) {
          snap_mode = (Editing::SnapMode) atoi (prop->value().c_str());
        }

return (snap_to);
}

void
TranzportControlProtocol::next_snapto_mode ()
{
  switch (snap_to) {
  case SnapToCDFrame: snap_to = SnapToSMPTEFrame;
  case SnapToSMPTEFrame: snap_to = SnapToSMPTESeconds; break;
  case SnapToSMPTESeconds: snap_to = SnapToSMPTEMinutes; break;
  case SnapToSMPTEMinutes: snap_to = SnapToSeconds; break;
  case SnapToSeconds: snap_to = SnapToMinutes; break;
  case SnapToMinutes: snap_to = SnapToBar; break;
  case SnapToBar: snap_to = SnapToBeat; break;
  case SnapToBeat: snap_to = SnapToAThirtysecondBeat; break;
  case SnapToAThirtysecondBeat: snap_to = SnapToASixteenthBeat; break;
  case SnapToASixteenthBeat: snap_to = SnapToAEighthBeat; break;
  case SnapToAEighthBeat: snap_to = SnapToAQuarterBeat; break;
  case SnapToAQuarterBeat: snap_to = SnapToAThirdBeat; break;
  case SnapToAThirdBeat: snap_to = SnapToMark; break;
  case SnapToMark: snap_to = SnapToCDFrame; break;
    // Haven't figured these out yet
  case SnapToRegionStart:
  case SnapToRegionEnd:
  case SnapToRegionSync:
  case SnapToRegionBoundary: 
  default: snap_to = SnapToSeconds; break;
  }

  notify("SnapTo:"); // FIXME convert to text
}

/*
        snprintf (buf, sizeof(buf), "%d", (int) snap_type);
        node->add_property ("snap-to", buf);
        snprintf (buf, sizeof(buf), "%d", (int) snap_mode);
        node->add_property ("snap-mode", buf);

       if (regions.size() == 1) {
                switch (snap_type) {
                case SnapToRegionStart:
                case SnapToRegionSync:
                case SnapToRegionEnd:
                        break;
                default:
                        snap_to (where);
                }
        } else {
                snap_to (where);
        }

*/


void
TranzportControlProtocol::go_snap_to (nframes64_t& start, int32_t direction, bool for_mark)
{
  if (!session || snap_mode == Editing::SnapOff) {
		return;
	}

	snap_to_internal (start, direction, for_mark);
}

/* Don't understand what these do yet

nframes64_t unit_to_frame (double unit) const {
  return (nframes64_t) rint (unit * frames_per_unit);
}

double frame_to_unit (nframes64_t frame) const {
  return rint ((double) frame / (double) frames_per_unit);
}

double frame_to_unit (double frame) const {
  return rint (frame / frames_per_unit);
}

*/


void
TranzportControlProtocol::snap_to_internal (nframes64_t& start, int32_t direction, bool for_mark)
{
  if(direction == 0 || !session) { return; }
  nframes64_t newstart = start;
  int32_t dir = 0 ;
  if(direction < 0) dir = -1;
  if(direction > 0) dir = 1;

  ARDOUR::Location* before = 0;
  ARDOUR::Location* after = 0;

  const nframes64_t one_second = session->frame_rate();
  const nframes64_t one_minute = session->frame_rate() * 60;
  const nframes64_t one_smpte_second = 
	(nframes64_t)(rint(session->smpte_frames_per_second()) *
	session->frames_per_smpte_frame());
  nframes64_t one_smpte_minute = 
	(nframes64_t)(rint(session->smpte_frames_per_second()) * 
	session->frames_per_smpte_frame() * 60);
  nframes64_t presnap = start;
  float speed = session->transport_speed();

  // FIXME: When the transport is moving it does you very little good
  // to try to move by CD frames or SMPTE Frames, or anything less than a bar.
  if(speed != 0.0) {
    switch(snap_to) {
    case Editing::SnapToSMPTESeconds: break;
    case Editing::SnapToSMPTEMinutes: break;
    case Editing::SnapToSeconds: break;
    case Editing::SnapToMinutes: break;
    case Editing::SnapToMark: break;
    case Editing::SnapToBar: break;

    case Editing::SnapToRegionStart: 
    case Editing::SnapToRegionEnd: 
    case Editing::SnapToRegionSync: 
    case Editing::SnapToRegionBoundary: notify("No snap to regions"); break;
    default: break;
    }
  } else {

  switch (snap_to) {
  case Editing::SnapToCDFrame:
    if(snap_mode == Editing::SnapOff) {
      FUDGE_64BIT_INC(start,dir); // move by samples instead
    } else {
      FUDGE_64BIT_INC(start,dir); // move by CD frames instead

      if (((dir == 0) && 
	   (start % (one_second/75) > (one_second/75) / 2)) || 
	   (dir > 0)) {
		start = (nframes64_t) ceil ((double) start / (one_second / 75)) * (one_second / 75);
      } else {
	start = (nframes64_t) floor ((double) start / (one_second / 75)) * (one_second / 75);
      }
    } 
    break;
    
  case Editing::SnapToSMPTEFrame:
      FUDGE_64BIT_INC(start,dir); 
    if (((dir == 0) && (fmod((double)start, (double)session->frames_per_smpte_frame()) > (session->frames_per_smpte_frame() / 2))) || (dir > 0)) {
      start = (nframes64_t) (ceil ((double) start / session->frames_per_smpte_frame()) * session->frames_per_smpte_frame());
    } else {
      start = (nframes64_t) (floor ((double) start / session->frames_per_smpte_frame()) *  session->frames_per_smpte_frame());
    }
    break;
    
  case Editing::SnapToSMPTESeconds:
    FUDGE_64BIT_INC(start,dir); 
    if (session->smpte_offset_negative())
      {
	start += session->smpte_offset ();
      } else {
      start -= session->smpte_offset ();
    }    
    if (((dir == 0) && (start % one_smpte_second > one_smpte_second / 2)) || dir > 0) {
      start = (nframes64_t) ceil ((double) start / one_smpte_second) * one_smpte_second;
    } else {
      start = (nframes64_t) floor ((double) start / one_smpte_second) * one_smpte_second;
    }
    
    if (session->smpte_offset_negative())
      {
	start -= session->smpte_offset ();
      } else {
      start += session->smpte_offset ();
    }
    break;
    
  case Editing::SnapToSMPTEMinutes:
    FUDGE_64BIT_INC(start,dir); 
    if (session->smpte_offset_negative())
      {
	start += session->smpte_offset ();
      } else {
      start -= session->smpte_offset ();
    }
    if (((dir == 0) && (start % one_smpte_minute > one_smpte_minute / 2)) || dir > 0) {
      start = (nframes64_t) ceil ((double) start / one_smpte_minute) * one_smpte_minute;
    } else {
      start = (nframes64_t) floor ((double) start / one_smpte_minute) * one_smpte_minute;
    }
    if (session->smpte_offset_negative())
      {
	start -= session->smpte_offset ();
      } else {
      start += session->smpte_offset ();
    }
    break;
    
  case Editing::SnapToSeconds:
    FUDGE_64BIT_INC(start,dir);
    if (((dir == 0) && (start % one_second > one_second / 2)) || (dir > 0)) {
      start = (nframes64_t) ceil ((double) start / one_second) * one_second;
    } else {
      start = (nframes64_t) floor ((double) start / one_second) * one_second;
    }
    break;
    
  case Editing::SnapToMinutes:
          FUDGE_64BIT_INC(start,dir);
if (((dir == 0) && (start % one_minute > one_minute / 2)) || (dir > 0)) {
      start = (nframes64_t) ceil ((double) start / one_minute) * one_minute;
    } else {
      start = (nframes64_t) floor ((double) start / one_minute) * one_minute;
    }
    break;
    
  case Editing::SnapToBar:
        FUDGE_64BIT_INC(start,dir);
	start = session->tempo_map().round_to_bar (start, dir);
    break;
    
  case Editing::SnapToBeat:
    FUDGE_64BIT_INC(start,dir);
    newstart = start;
    start = session->tempo_map().round_to_beat (start, dir);
    break;
    
  case Editing::SnapToAThirtysecondBeat:
    FUDGE_64BIT_INC(start,dir);
    start = session->tempo_map().round_to_beat_subdivision (start, 32);
    break;
    
  case Editing::SnapToASixteenthBeat:
    FUDGE_64BIT_INC(start,dir);
    start = session->tempo_map().round_to_beat_subdivision (start, 16);
    break;
    
  case Editing::SnapToAEighthBeat:
    FUDGE_64BIT_INC(start,dir);
    start = session->tempo_map().round_to_beat_subdivision (start, 8);
    break;
    
  case Editing::SnapToAQuarterBeat:
    FUDGE_64BIT_INC(start,dir);
    start = session->tempo_map().round_to_beat_subdivision (start, 4);
    break;
    
  case Editing::SnapToAThirdBeat:
    FUDGE_64BIT_INC(start,dir);
    start = session->tempo_map().round_to_beat_subdivision (start, 3);
    break;
    
  case Editing::SnapToMark:
    if (for_mark) {
      return;
    }
    FUDGE_64BIT_INC(start,dir);
    
    before = session->locations()->first_location_before (start);
    after = session->locations()->first_location_after (start);

    if (dir < 0) {
      if (before) {
	start = before->start();
      } else {
	start = 0;
      }
    } else if (dir > 0) {
      if (after) {
	start = after->start();
      } else {
	start = session->current_end_frame();
      }
    } else {
      if (before) {
	if (after) {
	  /* find nearest of the two */
	  if ((start - before->start()) < (after->start() - start)) {
	    start = before->start();
	  } else {
	    start = after->start();
	  }
	} else {
	  start = before->start();
	}
      } else if (after) {
	start = after->start();
      } else {
	/* relax */
      }
    }
    break;

  case Editing::SnapToRegionStart:
  case Editing::SnapToRegionEnd:
  case Editing::SnapToRegionSync:
  case Editing::SnapToRegionBoundary: notify("No snap to regions"); 
    /*		if (!region_boundary_cache.empty()) {
		vector<nframes64_t>::iterator i;
		
		if (direction > 0) {
		i = std::upper_bound (region_boundary_cache.begin(), region_boundary_cache.end(), start);
		} else {
		i = std::lower_bound (region_boundary_cache.begin(), region_boundary_cache.end(), start);
		}
		
		if (i != region_boundary_cache.end()) {
		
		// lower bound doesn't quite to the right thing for our purposes 
		
		if (direction < 0 && i != region_boundary_cache.begin()) {
		--i;
		}
		
		start = *i;
		
		} else {
		start = region_boundary_cache.back();
		}
		} 
    */
    break;
  }
  
/* don't pay attention to the snap_mode for now

	switch (snap_mode) {
	case Editing::SnapNormal:
		return;			
		
	case Editing::SnapMagnetic:
		
		if (presnap > start) {
			if (presnap > (start + unit_to_frame(snap_threshold))) {
				start = presnap;
			}
			
		} else if (presnap < start) {
			if (presnap < (start - unit_to_frame(snap_threshold))) {
				start = presnap;
			}
		}
		
	default:
		return;
		
	}
*/

/* FIXME: Recursion, with a reference var, no less. No mi gusta. 
   The right way to fix this would be to extend the various snapto
   calls in libardour to treat the direction argument as a count.

   This would also do nice things for the disk butler which otherwise
   is going nuts trying to keep up with a very rapid string of requests.
*/

	if(dir > 0) { 
	  --direction; // start++;  
	  snap_to_internal(start, direction, for_mark); 
	} else {
	  if(dir < 0) {
	    ++direction; // start--;
	  snap_to_internal(start, direction, for_mark); 
	  }
	}
  }
}


