/*
 * Copyright (c) 2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"

#include <sys/stat.h>

#include "np2.h"
#include "pccore.h"

#include "commng.h"
#include "sysmng.h"

#include "gtk/xnp2.h"
#include "gtk/gtk_menu.h"


static const char *mpu98_ioport_str[] = {
	"C0D0", "C4D0", "C8D0", "CCD0",
	"D0D0", "D4D0", "D8D0", "DCD0",
	"E0D0", "E4D0", "E8D0", "ECD0",
	"F0D0", "F4D0", "F8D0", "FCD0",
};

static const char *mpu98_intr_str[] = {
	"INT0", "INT1", "INT2", "INT5"
};

static const char *mpu98_devname_str[] = {
	"MIDI-OUT", "MIDI-IN"
};

static const char *mpu98_midiout_str[] = {
	"N/C",
	cmmidi_midiout_device,
#if defined(VERMOUTH_LIB)
	cmmidi_vermouth,
#endif
};

static const char *mpu98_midiin_str[] = {
	"N/C",
	cmmidi_midiin_device,
};

static GtkWidget *mpu98_ioport_entry;
static GtkWidget *mpu98_intr_entry;
static GtkWidget *mpu98_devname_entry[NELEMENTS(mpu98_devname_str)];
static GtkWidget *mpu98_midiout_entry;
static GtkWidget *mpu98_midiin_entry;
static GtkWidget *mpu98_module_entry;
static GtkWidget *mpu98_mimpi_def_checkbutton;
static GtkWidget *mpu98_mimpi_def_entry;
static BYTE mpuopt;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	const gchar *p;
	BOOL enable;
	UINT update;
	int i;

	UNUSED(b);

	update = 0;
	if (np2cfg.mpuopt != mpuopt) {
		update |= SYS_UPDATECFG | SYS_UPDATEMIDI;
	}
	p = gtk_entry_get_text(GTK_ENTRY(mpu98_midiout_entry));
	if (p != NULL) {
		if (milstr_cmp(np2oscfg.mpu.mout, p)) {
			milstr_ncpy(np2oscfg.mpu.mout, p, sizeof(np2oscfg.mpu.mout));
			update |= SYS_UPDATECFG | SYS_UPDATEMIDI;
		}
	}
	p = gtk_entry_get_text(GTK_ENTRY(mpu98_midiin_entry));
	if (p != NULL) {
		if (milstr_cmp(np2oscfg.mpu.min, p)) {
			milstr_ncpy(np2oscfg.mpu.min, p, sizeof(np2oscfg.mpu.min));
			update |= SYS_UPDATECFG | SYS_UPDATEMIDI;
		}
	}
	p = gtk_entry_get_text(GTK_ENTRY(mpu98_module_entry));
	if (p != NULL) {
		if (milstr_cmp(np2oscfg.mpu.mdl, p)) {
			milstr_ncpy(np2oscfg.mpu.mdl, p, sizeof(np2oscfg.mpu.mdl));
			update |= SYS_UPDATECFG | SYS_UPDATEMIDI;
		}
	}

	/* MIMPI def enable/file */
	enable = GTK_TOGGLE_BUTTON(mpu98_mimpi_def_checkbutton)->active ? 1 : 0;
	if (np2oscfg.mpu.def_en != enable) {
		np2oscfg.mpu.def_en = enable;
		if (cm_mpu98) {
			(*cm_mpu98->msg)(cm_mpu98, COMMSG_MIMPIDEFEN, enable);
		}
		update |= SYS_UPDATEOSCFG;
	}
	p = gtk_entry_get_text(GTK_ENTRY(mpu98_mimpi_def_entry));
	if (p != NULL) {
		if (milstr_cmp(np2oscfg.mpu.def, p)) {
			milstr_ncpy(np2oscfg.mpu.def, p, sizeof(np2oscfg.mpu.def));
			if (cm_mpu98) {
				(*cm_mpu98->msg)(cm_mpu98, COMMSG_MIMPIDEFFILE, (long)p);
			}
			update |= SYS_UPDATEOSCFG;
		}
	}

	/* MIDI-IN/OUT device */
	for (i = 0; i < NELEMENTS(mpu98_devname_str); i++) {
		p = gtk_entry_get_text(GTK_ENTRY(mpu98_devname_entry[i]));
		if (p != NULL) {
			if (milstr_cmp(np2oscfg.MIDIDEV[i], p)) {
				milstr_ncpy(np2oscfg.MIDIDEV[i], p, sizeof(np2oscfg.MIDIDEV[0]));
				update |= SYS_UPDATEOSCFG;
			}
		}
	}

	if (update) {
		sysmng_update(update);
	}

	gtk_widget_destroy((GtkWidget *)d);
}

static void
dialog_destroy(GtkWidget *w, GtkWidget **wp)
{

	UNUSED(wp);

	install_idle_process();
	gtk_widget_destroy(w);
}

static void
mpu98_ioport_entry_changed(GtkEditable *e, gpointer d)
{
	const gchar *p;
	BYTE val;

	UNUSED(d);
	p = gtk_entry_get_text(GTK_ENTRY(e));
	if ((p != NULL) && (strlen(p) >= 4)) {
		val = (milstr_solveHEX(p) >> 6) & 0xf0;
		mpuopt &= ~0xf0;
		mpuopt |= val;
	}
}

static void
mpu98_intr_entry_changed(GtkEditable *e, gpointer d)
{
	const gchar *p;
	BYTE val;

	UNUSED(d);

	p = gtk_entry_get_text(GTK_ENTRY(e));
	if ((p != NULL) && (strlen(p) >= 4)) {
		val = p[3] - '0';
		if (val >= 3)
			val = 3;
		mpuopt &= ~0x03;
		mpuopt |= val;
	}
}

static void
mpu98_default_button_clicked(GtkButton *b, gpointer d)
{

	UNUSED(b);
	UNUSED(d);

	gtk_entry_set_text(GTK_ENTRY(mpu98_ioport_entry), "E0D0");
	gtk_entry_set_text(GTK_ENTRY(mpu98_intr_entry), "INT2");
}

/* ----- MIMPI */
static BOOL
mpu98_mimpi_def_dialog_ok_cb(void *arg, const char *path)
{
	struct stat sb;
	BOOL result = FALSE;

	if (stat(path, &sb) == 0) {
		if (S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
			gtk_entry_set_text(GTK_ENTRY(arg), path);
			result = TRUE;
		}
	}

	return result;
}

static void
mpu98_mimpi_def_button_clicked(GtkButton *b, gpointer d)
{

	UNUSED(b);
	UNUSED(d);

	create_file_selection("Open MIMPI define file", np2oscfg.mpu.def,
	    mpu98_mimpi_def_entry, mpu98_mimpi_def_dialog_ok_cb, NULL);
}

void
create_midi_dialog(void)
{
	GtkWidget *midi_dialog;
	GtkWidget *main_widget;
	GtkWidget *ioport_label;
	GtkWidget *ioport_combo;
	GtkWidget *intr_label;
	GtkWidget *intr_combo;
	GtkWidget *device_frame;
	GtkWidget *deviceframe_widget;
	GtkWidget *devname_label[NELEMENTS(mpu98_devname_str)];
	GtkWidget *assign_frame;
	GtkWidget *assignframe_widget;
	GtkWidget *midiout_label;
	GtkWidget *midiout_combo;
	GtkWidget *midiin_label;
	GtkWidget *midiin_combo;
	GtkWidget *module_label;
	GtkWidget *module_combo;
	GtkWidget *mimpi_button;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *mpu98_default_button;
	GList *ioport_combo_items = NULL;
	GList *intr_combo_items = NULL;
	GList *midiout_combo_items = NULL;
	GList *midiin_combo_items = NULL;
	GList *module_combo_items = NULL;
	int i;

	uninstall_idle_process();

	midi_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(midi_dialog), "MPU-PC98II");
	gtk_window_set_position(GTK_WINDOW(midi_dialog),GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(midi_dialog), TRUE);
	gtk_window_set_policy(GTK_WINDOW(midi_dialog), FALSE, FALSE, FALSE);

	gtk_signal_connect(GTK_OBJECT(midi_dialog), "destroy",
	    GTK_SIGNAL_FUNC(dialog_destroy), NULL);

	main_widget = gtk_table_new(10, 6, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(main_widget), 5);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(midi_dialog), main_widget);

	/*
	 * I/O port
	 */
	ioport_label = gtk_label_new("I/O port");
	gtk_widget_show(ioport_label);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), ioport_label,
	    0, 1, 0, 1);

	ioport_combo = gtk_combo_new();
	gtk_widget_set_usize(ioport_combo, 80, 0);
	gtk_widget_show(ioport_combo);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), ioport_combo,
	    1, 2, 0, 1);
	gtk_combo_set_value_in_list(GTK_COMBO(ioport_combo), TRUE, TRUE);
	gtk_combo_set_use_arrows_always(GTK_COMBO(ioport_combo), TRUE);
	for (i = 0; i < NELEMENTS(mpu98_ioport_str); i++) {
		ioport_combo_items = g_list_append(ioport_combo_items,
		    (gpointer)mpu98_ioport_str[i]);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(ioport_combo),
	    ioport_combo_items);
	g_list_free(ioport_combo_items);

	mpu98_ioport_entry = GTK_COMBO(ioport_combo)->entry;
	gtk_widget_show(mpu98_ioport_entry);
	gtk_signal_connect(GTK_OBJECT(mpu98_ioport_entry), "changed",
	    GTK_SIGNAL_FUNC(mpu98_ioport_entry_changed), NULL);
	gtk_entry_set_editable(GTK_ENTRY(mpu98_ioport_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(mpu98_ioport_entry),
	    mpu98_ioport_str[(np2cfg.mpuopt >> 4) & 0x0f]);

	/*
	 * Interrupt
	 */
	intr_label = gtk_label_new("Interrupt");
	gtk_widget_show(intr_label);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), intr_label,
	    0, 1, 1, 2);

	intr_combo = gtk_combo_new();
	gtk_widget_set_usize(intr_combo, 80, 0);
	gtk_widget_show(intr_combo);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), intr_combo,
	    1, 2, 1, 2);
	gtk_combo_set_value_in_list(GTK_COMBO(intr_combo), TRUE, TRUE);
	gtk_combo_set_use_arrows_always(GTK_COMBO(intr_combo), TRUE);
	for (i = 0; i < NELEMENTS(mpu98_intr_str); i++) {
		intr_combo_items = g_list_append(intr_combo_items,
		    (gpointer)mpu98_intr_str[i]);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(intr_combo), intr_combo_items);
	g_list_free(intr_combo_items);

	mpu98_intr_entry = GTK_COMBO(intr_combo)->entry;
	gtk_widget_show(mpu98_intr_entry);
	gtk_signal_connect(GTK_OBJECT(mpu98_intr_entry), "changed",
	    GTK_SIGNAL_FUNC(mpu98_intr_entry_changed), NULL);
	gtk_entry_set_editable(GTK_ENTRY(mpu98_intr_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(mpu98_intr_entry),
	    mpu98_intr_str[np2cfg.mpuopt & 0x03]);

	/*
	 * MIDI-IN/OUT device
	 */
	device_frame = gtk_frame_new("Device");
	gtk_container_set_border_width(GTK_CONTAINER(device_frame), 2);
	gtk_widget_show(device_frame);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), device_frame,
	    0, 6, 2, 4);

	deviceframe_widget = gtk_table_new(2, 6, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(deviceframe_widget), 5);
	gtk_widget_show(deviceframe_widget);
	gtk_table_set_row_spacings(GTK_TABLE(deviceframe_widget), 3);
	gtk_table_set_col_spacings(GTK_TABLE(deviceframe_widget), 3);
	gtk_container_add(GTK_CONTAINER(device_frame), deviceframe_widget);

	for (i = 0; i < NELEMENTS(mpu98_devname_str); i++) {
		devname_label[i] = gtk_label_new(mpu98_devname_str[i]);
		gtk_widget_show(devname_label[i]);
		gtk_table_attach_defaults(GTK_TABLE(deviceframe_widget),
		    devname_label[i], 0, 1, i, i + 1);

		mpu98_devname_entry[i] = gtk_entry_new();
		gtk_widget_show(mpu98_devname_entry[i]);
		gtk_table_attach_defaults(GTK_TABLE(deviceframe_widget),
		    mpu98_devname_entry[i], 1, 6, i, i + 1);

		gtk_entry_set_text(GTK_ENTRY(mpu98_devname_entry[i]),
		    np2oscfg.MIDIDEV[i]);
	}
	/* MIDI-IN disable */
	gtk_widget_set_sensitive(mpu98_devname_entry[1], FALSE);

	/* assign */
	assign_frame = gtk_frame_new("Assign");
	gtk_container_set_border_width(GTK_CONTAINER(assign_frame), 2);
	gtk_widget_show(assign_frame);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), assign_frame,
	    0, 6, 4, 10);

	assignframe_widget = gtk_table_new(5, 6, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(assignframe_widget), 5);
	gtk_widget_show(assignframe_widget);
	gtk_table_set_row_spacings(GTK_TABLE(assignframe_widget), 3);
	gtk_table_set_col_spacings(GTK_TABLE(assignframe_widget), 3);
	gtk_container_add(GTK_CONTAINER(assign_frame), assignframe_widget);

	/*
	 * MIDI-OUT
	 */
	midiout_label = gtk_label_new("MIDI-OUT");
	gtk_widget_show(midiout_label);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), midiout_label,
	    0, 1, 0, 1);

	midiout_combo = gtk_combo_new();
	gtk_widget_show(midiout_combo);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), midiout_combo,
	    1, 6, 0, 1);
	gtk_combo_set_value_in_list(GTK_COMBO(midiout_combo), TRUE, TRUE);
	gtk_combo_set_use_arrows_always(GTK_COMBO(midiout_combo), TRUE);
	for (i = 0; i < NELEMENTS(mpu98_midiout_str); i++) {
		midiout_combo_items = g_list_append(midiout_combo_items,
		    (gpointer)mpu98_midiout_str[i]);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(midiout_combo),
	    midiout_combo_items);
	g_list_free(midiout_combo_items);

	mpu98_midiout_entry = GTK_COMBO(midiout_combo)->entry;
	gtk_widget_show(mpu98_midiout_entry);
	gtk_entry_set_editable(GTK_ENTRY(mpu98_midiout_entry), FALSE);
	for (i = 0; i < NELEMENTS(mpu98_midiout_str); i++) {
		if (!milstr_extendcmp(np2oscfg.mpu.mout, mpu98_midiout_str[i])){
			break;
		}
	}
	if (i < NELEMENTS(mpu98_midiout_str)) {
		gtk_entry_set_text(GTK_ENTRY(mpu98_midiout_entry),
		    mpu98_midiout_str[i]);
	} else {
		gtk_entry_set_text(GTK_ENTRY(mpu98_midiout_entry), "N/C");
		if (np2oscfg.mpu.mout[0] != '\0') {
			np2oscfg.mpu.mout[0] = '\0';
			sysmng_update(SYS_UPDATECFG|SYS_UPDATEMIDI);
		}
	}

	/*
	 * MIDI-IN
	 */
	midiin_label = gtk_label_new("MIDI-IN");
	gtk_widget_show(midiin_label);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), midiin_label,
	    0, 1, 1, 2);

	midiin_combo = gtk_combo_new();
	gtk_widget_show(midiin_combo);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), midiin_combo,
	    1, 6, 1, 2);
	gtk_combo_set_value_in_list(GTK_COMBO(midiin_combo), TRUE, TRUE);
	gtk_combo_set_use_arrows_always(GTK_COMBO(midiin_combo), TRUE);
	for (i = 0; i < NELEMENTS(mpu98_midiin_str); i++) {
		midiin_combo_items = g_list_append(midiin_combo_items,
		    (gpointer)mpu98_midiin_str[i]);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(midiin_combo),
	    midiin_combo_items);
	g_list_free(midiin_combo_items);

	mpu98_midiin_entry = GTK_COMBO(midiin_combo)->entry;
	gtk_widget_show(mpu98_midiin_entry);
	gtk_entry_set_editable(GTK_ENTRY(mpu98_midiin_entry), FALSE);
	for (i = 0; i < NELEMENTS(mpu98_midiin_str); i++) {
		if (!milstr_extendcmp(np2oscfg.mpu.min, mpu98_midiin_str[i])){
			break;
		}
	}
	if (i < NELEMENTS(mpu98_midiin_str)) {
		gtk_entry_set_text(GTK_ENTRY(mpu98_midiin_entry),
		    mpu98_midiin_str[i]);
	} else {
		gtk_entry_set_text(GTK_ENTRY(mpu98_midiin_entry), "N/C");
		if (np2oscfg.mpu.min[0] != '\0') {
			np2oscfg.mpu.min[0] = '\0';
			sysmng_update(SYS_UPDATECFG|SYS_UPDATEMIDI);
		}
	}
	/* MIDI-IN disable */
	gtk_widget_set_sensitive(midiin_combo, FALSE);

	/*
	 * Module
	 */
	module_label = gtk_label_new("Module");
	gtk_widget_show(module_label);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), module_label,
	    0, 1, 2, 3);

	module_combo = gtk_combo_new();
	gtk_widget_show(module_combo);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget), module_combo,
	    1, 6, 2, 3);
	gtk_combo_set_value_in_list(GTK_COMBO(module_combo), TRUE, TRUE);
	gtk_combo_set_use_arrows_always(GTK_COMBO(module_combo), TRUE);
	for (i = 0; i < MIDI_OTHER; i++) {
		module_combo_items = g_list_append(module_combo_items,
		    (gpointer)cmmidi_mdlname[i]);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(module_combo),
	    module_combo_items);
	g_list_free(module_combo_items);

	mpu98_module_entry = GTK_COMBO(module_combo)->entry;
	gtk_widget_show(mpu98_module_entry);
	gtk_entry_set_editable(GTK_ENTRY(mpu98_module_entry), TRUE);
	gtk_entry_set_text(GTK_ENTRY(mpu98_module_entry), np2oscfg.mpu.mdl);

	/*
	 * MIMPI def
	 */
	mpu98_mimpi_def_checkbutton = gtk_check_button_new_with_label(
	    "Use program define file (MIMPI define)");
	gtk_widget_show(mpu98_mimpi_def_checkbutton);
	gtk_table_attach_defaults(GTK_TABLE(assignframe_widget),
	    mpu98_mimpi_def_checkbutton, 0, 4, 3, 4);

	mpu98_mimpi_def_entry = gtk_entry_new();
	gtk_widget_show(mpu98_mimpi_def_entry);
	gtk_table_attach(GTK_TABLE(assignframe_widget), mpu98_mimpi_def_entry,
	    0, 5, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_widget_set_sensitive(mpu98_mimpi_def_entry, FALSE);
	gtk_entry_set_text(GTK_ENTRY(mpu98_mimpi_def_entry), np2oscfg.mpu.def);

	mimpi_button = gtk_button_new_with_label("...");
	gtk_widget_show(mimpi_button);
	gtk_table_attach(GTK_TABLE(assignframe_widget), mimpi_button,
	    5, 6, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_signal_connect(GTK_OBJECT(mimpi_button), "clicked",
	    GTK_SIGNAL_FUNC(mpu98_mimpi_def_button_clicked), 0);

	/*
	 * "Default" button
	 */
	mpu98_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(mpu98_default_button);
	gtk_table_attach(GTK_TABLE(main_widget), mpu98_default_button,
	    2, 3, 1, 2, GTK_SHRINK, GTK_SHRINK, 5, 5);
	gtk_signal_connect_object(GTK_OBJECT(mpu98_default_button), "clicked",
	    GTK_SIGNAL_FUNC(mpu98_default_button_clicked), NULL);

	/*
	 * OK, Cancel button
	 */
	confirm_widget = gtk_vbutton_box_new();
	gtk_widget_show(confirm_widget);
	gtk_table_attach(GTK_TABLE(main_widget), confirm_widget, 4, 6, 0, 2,
	    GTK_FILL, GTK_FILL, 0, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_widget),
	    GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(confirm_widget), 0);

	ok_button = gtk_button_new_with_label("OK");
	gtk_widget_set_usize(ok_button, 80, 0);
	gtk_widget_show(ok_button);
	gtk_container_add(GTK_CONTAINER(confirm_widget), ok_button);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
	    GTK_SIGNAL_FUNC(ok_button_clicked), (gpointer)midi_dialog);
	gtk_widget_grab_default(ok_button);

	cancel_button = gtk_button_new_with_label("Cancel");
	gtk_widget_set_usize(cancel_button, 80, 0);
	gtk_widget_show(cancel_button);
	gtk_container_add(GTK_CONTAINER(confirm_widget), cancel_button);
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(cancel_button), "clicked",
	    GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(midi_dialog));

	gtk_widget_show_all(midi_dialog);
}
