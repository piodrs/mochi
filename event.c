#include <X11/Xlib.h>
#include "client.h"
#include "display.h"
#include "event.h"
#include "frame.h"
#include "input.h"
#include "keys.h"
#include "session.h"

void event_dispatch(XEvent *event)
{
	switch (event->type) {
	case MapRequest:
		client_map(event->xmaprequest.window);
		break;
	case ConfigureRequest:
		client_configure(&event->xconfigurerequest);
		break;
	case UnmapNotify:
		client_unmap(&event->xunmap);
		break;
	case DestroyNotify:
		client_destroy(event->xdestroywindow.window);
		break;
	case KeyPress:
		fish.time = event->xkey.time;
		input_key(&event->xkey);
		break;
	case Expose:
		if (event->xexpose.window == display_window())
			input_draw();
		break;
	case MappingNotify:
		XRefreshKeyboardMapping(&event->xmapping);
		keys_refresh();
		break;
	case ConfigureNotify:
		if (event->xconfigure.window == fish.root) {
			frame_layout(fish.tree, 0, 0, event->xconfigure.width,
				     event->xconfigure.height);
			client_refresh();
		}
		break;
	case PropertyNotify:
		client_property(&event->xproperty);
		break;
	case ClientMessage:
		client_message(&event->xclient);
		break;
	}
}
