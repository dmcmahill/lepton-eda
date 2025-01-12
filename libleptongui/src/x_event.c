/* Lepton EDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2015 gEDA Contributors
 * Copyright (C) 2017-2023 Lepton EDA Contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <config.h>

#include <stdio.h>
#include <math.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "gschem.h"
#include <gdk/gdkkeysyms.h>


/* used for the stroke stuff */
#ifdef HAVE_LIBSTROKE
static int DOING_STROKE = FALSE;
#endif /* HAVE_LIBSTROKE */

gboolean
schematic_event_get_doing_stroke ()
{
#ifdef HAVE_LIBSTROKE
  return DOING_STROKE;
#else
  return FALSE;
#endif /* HAVE_LIBSTROKE */
}

void
schematic_event_set_doing_stroke (gboolean val)
{
#ifdef HAVE_LIBSTROKE
  DOING_STROKE = val;
#endif /* HAVE_LIBSTROKE */
}


gint
schematic_event_shift_mask ()
{
  return GDK_SHIFT_MASK;
}


gint
schematic_event_control_mask ()
{
  return GDK_CONTROL_MASK;
}


gint
schematic_event_alt_mask ()
{
  return GDK_MOD1_MASK;
}

gboolean
schematic_event_is_double_button_press (GdkEvent *event)
{
#ifdef ENABLE_GTK3
  return gdk_event_get_event_type (event) == GDK_2BUTTON_PRESS;
#else
  return ((GdkEventButton*) event)->type == GDK_2BUTTON_PRESS;
#endif
}


guint
schematic_event_get_button (GdkEvent *event)
{
#ifdef ENABLE_GTK3
  guint button;
  gdk_event_get_button (event, &button);
  return button;
#else
  return ((GdkEventButton*) event)->button;
#endif
}


#ifdef ENABLE_GTK3
/*! \brief Redraws the view when widget is exposed.
 *
 *  \param [in] view      The GschemPageView.
 *  \param [in] cr        The cairo context.
 *  \param [in] w_current The GschemToplevel.
 *  \returns FALSE to propagate the event further.
 */
gint
x_event_draw (GschemPageView *view,
              cairo_t *cr,
              GschemToplevel *w_current)
{
  gschem_page_view_redraw (view, cr, w_current);

  return(0);
}


/* Dummy function for making Scheme happy. */
gint
x_event_expose (gpointer view,
                gpointer event,
                gpointer w_current)
{
  return(0);
}


#else /* GTK2 */


/* Dummy function for making Scheme happy. */
gint
x_event_draw (gpointer view,
              gpointer cr,
              gpointer w_current)
{
  return(0);
}


/*! \brief Redraws the view when widget is exposed.
 *
 *  \param [in] view      The GschemPageView.
 *  \param [in] event     The event structure.
 *  \param [in] w_current The GschemToplevel.
 *  \returns FALSE to propagate the event further.
 */
gint
x_event_expose (GschemPageView *view,
                GdkEventExpose *event,
                GschemToplevel *w_current)
{
  gschem_page_view_redraw (view, event, w_current);

  return(0);
}
#endif


/*! \brief Check if a moving event has to be skipped.
 *
 *  \par Function Description
 *
 *  The function checks if there are more moving events in the GDK
 *  event queue.  If the event in the queue is a motion event and
 *  has the same state (depressed buttons and modifiers are the
 *  same) as \a event has, the function returns TRUE.  Otherwise
 *  it returns FALSE.
 *
 *  \param event The event to compare state with.
 *  \returns TRUE if event in the queue matches, otherwise FALSE.
 */

gboolean
schematic_event_skip_motion_event (GdkEvent *event)
{
  gboolean skip_event = FALSE;
  GdkEvent *test_event;

  if ((test_event = gdk_event_get()) != NULL)
  {
    GdkModifierType state;
    gdk_event_get_state (event, &state);

    /* Only skip the event if it is a motion event and no buttons
     * or modifier keys changed. */
    if (test_event->type == GDK_MOTION_NOTIFY
        && ((GdkEventMotion *) test_event)->state == state)
    {
      skip_event = TRUE;
    }
    /* Put it back in front of the queue. */
    gdk_event_put (test_event);
    gdk_event_free (test_event);
  }

  return skip_event;
}


/*! \brief Updates the display when drawing area is configured.
 *  \par Function Description
 *  This is the callback function connected to the configure event of
 *  the GschemPageView of the main window.
 *
 *  It re-pans each of its pages to keep their contents centered in the
 *  GschemPageView.
 *
 *  When the window is maximised, the zoom of every page is changed to
 *  best fit the previously displayed area of the page in the new
 *  area. Otherwise the current zoom level is left unchanged.
 *
 *  \param [in] widget    The GschemPageView which received the signal.
 *  \param [in] event     The event structure of signal configure-event.
 *  \param [in] unused
 *  \returns FALSE to propagate the event further.
 */
gboolean
x_event_configure (GschemPageView    *page_view,
                   GdkEventConfigure *event,
                   gpointer           unused)
{
  GtkAllocation current_allocation;
  GList *iter;
  LeptonPage *p_current = gschem_page_view_get_page (page_view);

  if (p_current == NULL) {
    /* don't want to call this if the current page isn't setup yet */
    return FALSE;
  }

  g_return_val_if_fail (p_current->toplevel != NULL, FALSE);

  gtk_widget_get_allocation (GTK_WIDGET(page_view), &current_allocation);

  if ((current_allocation.width == page_view->previous_allocation.width) &&
      (current_allocation.height == page_view->previous_allocation.height)) {
    /* the size of the drawing area has not changed -- nothing to do here */
    return FALSE;
  }

  page_view->previous_allocation = current_allocation;


  /* tabbed GUI: zoom/pan, mark page_view as configured and return:
   * there is only one page per page view.
  */
  if (x_tabs_enabled())
  {
    if (page_view->configured)
    {
      gschem_page_view_pan_mouse (page_view, 0, 0);
    }
    else
    {
      gschem_page_view_zoom_extents (page_view, NULL);
    }

    page_view->configured = TRUE;
    return FALSE;
  }


  /* re-pan each page of the LeptonToplevel */
  for ( iter = lepton_list_get_glist (p_current->toplevel->pages);
        iter != NULL;
        iter = g_list_next (iter) ) {

    gschem_page_view_set_page (page_view, (LeptonPage *)iter->data);

    if (page_view->configured) {
      gschem_page_view_pan_mouse (page_view, 0, 0);
    } else {
      gschem_page_view_zoom_extents (page_view, NULL);
    }
  }

  page_view->configured = TRUE;

  gschem_page_view_set_page (page_view, p_current);

  return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
gint x_event_enter(GtkWidget *widget, GdkEventCrossing *event,
                   GschemToplevel *w_current)
{
  g_return_val_if_fail ((w_current != NULL), 0);
  /* do nothing or now */
  return(0);
}

/*! \brief Callback to handle key events in the drawing area.
 *  \par Function Description
 *
 *  GTK+ callback function (registered in x_window_setup_draw_events() ) which
 *  handles key press and release events from the GTK+ system.
 *
 * \param [in] widget     the widget that generated the event
 * \param [in] event      the event itself
 * \param      w_current  the toplevel environment
 * \returns TRUE if the event has been handled.
 */
GdkEventKey*
x_event_key (GschemPageView *page_view,
             GdkEventKey *event,
             GschemToplevel *w_current)
{
  int pressed;
  gboolean special = FALSE;

  g_return_val_if_fail (page_view != NULL, FALSE);

#if DEBUG
  printf("x_event_key_pressed: Pressed key %i.\n", event->keyval);
#endif

  /* update the state of the modifiers */
  w_current->ALTKEY     = (event->state & GDK_MOD1_MASK)    ? 1 : 0;
  w_current->SHIFTKEY   = (event->state & GDK_SHIFT_MASK)   ? 1 : 0;
  w_current->CONTROLKEY = (event->state & GDK_CONTROL_MASK) ? 1 : 0;

  pressed = (event->type == GDK_KEY_PRESS) ? 1 : 0;

  switch (event->keyval) {
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
      w_current->ALTKEY = pressed;
      break;

    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
      w_current->SHIFTKEY = pressed;
      special = TRUE;
      break;

    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
      w_current->CONTROLKEY = pressed;
      special = TRUE;
      break;
  }

  /* Special case to update the object being drawn or placed after
   * scrolling when Shift or Control were pressed */
  if (special) {
    x_event_faked_motion (page_view, event);
  }

  return pressed ? event : NULL;
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \param [in] widget The GschemPageView with the scroll event.
 *  \param [in] event
 *  \param [in] w_current
 */
gint x_event_scroll (GtkWidget *widget, GdkEventScroll *event,
                     GschemToplevel *w_current)
{
  GtkAdjustment *adj;
  gboolean pan_xaxis = FALSE;
  gboolean pan_yaxis = FALSE;
  gboolean zoom = FALSE;
  int pan_direction = 1;
  int zoom_direction = ZOOM_IN;
  GschemPageView *view = NULL;
  LeptonPage *page = NULL;

  g_return_val_if_fail ((w_current != NULL), 0);

  view = GSCHEM_PAGE_VIEW (widget);
  g_return_val_if_fail ((view != NULL), 0);

  page = gschem_page_view_get_page (view);

  if (page == NULL) {
    return FALSE; /* we cannot zoom page if it doesn't exist :) */
  }

  /* update the state of the modifiers */
  w_current->SHIFTKEY   = (event->state & GDK_SHIFT_MASK  ) ? 1 : 0;
  w_current->CONTROLKEY = (event->state & GDK_CONTROL_MASK) ? 1 : 0;
  w_current->ALTKEY     = (event->state & GDK_MOD1_MASK) ? 1 : 0;

  if (w_current->scroll_wheel == SCROLL_WHEEL_CLASSIC) {
    /* Classic gschem behaviour */
    zoom =      !w_current->CONTROLKEY && !w_current->SHIFTKEY;
    pan_yaxis = !w_current->CONTROLKEY &&  w_current->SHIFTKEY;
    pan_xaxis =  w_current->CONTROLKEY && !w_current->SHIFTKEY;
  } else {
    /* GTK style behaviour */
    zoom =       w_current->CONTROLKEY && !w_current->SHIFTKEY;
    pan_yaxis = !w_current->CONTROLKEY && !w_current->SHIFTKEY;
    pan_xaxis = !w_current->CONTROLKEY &&  w_current->SHIFTKEY;
  }

  /* If the user has a left/right scroll wheel, always scroll the y-axis */
  if (event->direction == GDK_SCROLL_LEFT ||
      event->direction == GDK_SCROLL_RIGHT) {
    zoom = FALSE;
    pan_yaxis = FALSE;
    pan_xaxis = TRUE;
  }

  /* You must have scrollbars enabled if you want to use the scroll wheel to pan */
  if (!w_current->scrollbars_flag) {
    pan_xaxis = FALSE;
    pan_yaxis = FALSE;
  }

#ifdef ENABLE_GTK3
  static guint last_scroll_event_time = GDK_CURRENT_TIME;
  /* check for duplicate legacy scroll event, see GNOME bug 726878 */
  if (event->direction != GDK_SCROLL_SMOOTH &&
      last_scroll_event_time == event->time) {
    g_debug ("[%d] duplicate legacy scroll event %d\n",
             event->time,
             event->direction);
    return FALSE;
  }

  switch (event->direction) {
  case GDK_SCROLL_SMOOTH:
    /* As of GTK 3.4, all directional scroll events are provided by */
    /* the GDK_SCROLL_SMOOTH direction on XInput2 and Wayland devices. */
    last_scroll_event_time = event->time;

    /* event->delta_x seems to be unused on not touch devices. */
    pan_direction = event->delta_y;
    zoom_direction = (event->delta_y > 0) ? ZOOM_OUT : ZOOM_IN;
    break;
  case GDK_SCROLL_UP:
  case GDK_SCROLL_LEFT:
    pan_direction = -1;
    zoom_direction = ZOOM_IN;
    break;
  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_RIGHT:
    pan_direction =  1;
    zoom_direction = ZOOM_OUT;
    break;
  }
#else
  switch (event->direction) {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
      pan_direction = -1;
      zoom_direction = ZOOM_IN;
      break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
      pan_direction =  1;
      zoom_direction = ZOOM_OUT;
      break;
  }
#endif

  if (zoom) {
    /*! \todo Change "HOTKEY" TO new "MOUSE" specifier? */
    a_zoom(w_current, GSCHEM_PAGE_VIEW (widget), zoom_direction, HOTKEY);
  }

  if (pan_xaxis) {
    adj = gschem_page_view_get_hadjustment (GSCHEM_PAGE_VIEW (widget));
    g_return_val_if_fail (adj != NULL, TRUE);
    gtk_adjustment_set_value (adj,
                              MIN (gtk_adjustment_get_value (adj) + pan_direction *
                                   (gtk_adjustment_get_page_increment (adj) /
                                    w_current->scrollpan_steps),
                                   gtk_adjustment_get_upper (adj) -
                                   gtk_adjustment_get_page_size (adj)));
  }

  if (pan_yaxis) {
    adj = gschem_page_view_get_vadjustment (GSCHEM_PAGE_VIEW (widget));
    g_return_val_if_fail (adj != NULL, TRUE);
    gtk_adjustment_set_value (adj,
                              MIN (gtk_adjustment_get_value (adj) + pan_direction *
                                   (gtk_adjustment_get_page_increment (adj) /
                                    w_current->scrollpan_steps),
                                   gtk_adjustment_get_upper (adj) -
                                   gtk_adjustment_get_page_size (adj)));
  }

  if (w_current->undo_panzoom && (zoom || pan_xaxis || pan_yaxis)) {
    o_undo_savestate_viewport (w_current);
  }

  x_event_faked_motion (view, NULL);
  /* Stop further processing of this signal */
  return TRUE;
}


/*! \brief get the pointer position of a given GschemToplevel
 *  \par Function Description
 *  This function gets the pointer position of the drawing area of the
 *  current workspace <b>GschemToplevel</b>. The flag <b>snapped</b> specifies
 *  whether the pointer position should be snapped to the current grid.
 *
 *  \param [in] w_current  The GschemToplevel object.
 *  \param [in] snapped    An option flag to specify the wished coords
 *  \param [out] wx        snapped/unsnapped world x coordinate
 *  \param [out] wy        snapped/unsnapped world y coordinate
 *
 *  \return Returns TRUE if the pointer position is inside the drawing area.
 *
 */
gboolean
x_event_get_pointer_position (GschemToplevel *w_current, gboolean snapped, gint *wx, gint *wy)
{
  int width;
  int height;
  int sx;
  int sy;
  int x;
  int y;

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_val_if_fail (page_view != NULL, FALSE);

  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (page_view));
  g_return_val_if_fail (window != NULL, FALSE);

  width = gdk_window_get_width (window);
  height = gdk_window_get_height (window);

#ifdef ENABLE_GTK3
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *pointer = gdk_seat_get_pointer (seat);

  gdk_window_get_device_position (window, pointer, &sx, &sy, NULL);
#else
  gtk_widget_get_pointer(GTK_WIDGET (page_view), &sx, &sy);
#endif

  /* check if we are inside the drawing area */
  if ((sx < 0) || (sx >= width) || (sy < 0) || (sy >= height)) {
    return FALSE;
  }

  gschem_page_view_SCREENtoWORLD (page_view, sx, sy, &x, &y);

  if (snapped) {
    x = snap_grid (w_current, x);
    y = snap_grid (w_current, y);
  }

  *wx = x;
  *wy = y;

  return TRUE;
}

/*! \brief Emits a faked motion event to update objects being drawn or placed
 *  \par Function Description
 *  This function emits an additional "motion-notify-event" to
 *  update objects being drawn or placed while zooming, scrolling, or
 *  panning.
 *
 *  If its event parameter is not NULL, the current state of Shift
 *  and Control is preserved to correctly deal with special cases.
 *
 *  \param [in] view      The GschemPageView object which received the signal.
 *  \param [in] event     The event structure of the signal or NULL.
 *  \returns FALSE to propagate the event further.
 */
gboolean
x_event_faked_motion (GschemPageView *view, GdkEventKey *event) {
  gint x, y;
  gboolean ret;
  GdkEventMotion *newevent;

#ifdef ENABLE_GTK3
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (view));
  g_return_val_if_fail (window != NULL, FALSE);

  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *pointer = gdk_seat_get_pointer (seat);

  gdk_window_get_device_position (window, pointer, &x, &y, NULL);
#else
  gtk_widget_get_pointer (GTK_WIDGET (view), &x, &y);
#endif
  newevent = (GdkEventMotion*)gdk_event_new(GDK_MOTION_NOTIFY);
  newevent->x = x;
  newevent->y = y;

  if (event != NULL ) {
    switch (event->keyval) {
      case GDK_KEY_Control_L:
      case GDK_KEY_Control_R:
        if (event->type == GDK_KEY_PRESS) {
          newevent->state |= GDK_CONTROL_MASK;
        } else {
          newevent->state &= ~GDK_CONTROL_MASK;
        }
        break;

      case GDK_KEY_Shift_L:
      case GDK_KEY_Shift_R:
        if (event->type == GDK_KEY_PRESS) {
          newevent->state |= GDK_SHIFT_MASK;
        } else {
          newevent->state &= ~GDK_SHIFT_MASK;
        }
        break;
    }
  }

  g_signal_emit_by_name (view, "motion-notify-event", newevent, &ret);

  gdk_event_free((GdkEvent*)newevent);

  return FALSE;
}
