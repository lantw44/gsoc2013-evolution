/*
 * 
 * Authors: Srinivasa Ragavan <sragavan@novell.com>
 *
 * */


#ifndef CAMEL_OBJECT_REMOTE_IMPL_H
#define CAMEL_OBJECT_REMOTE_IMPL_H


#include <camel/camel.h>

#define CAMEL_SESSION_OBJECT_PATH "/org/gnome/evolution/camel/session"
#define MAIL_SESSION_OBJECT_PATH "/org/gnome/evolution/camel/session/mail"
#define CAMEL_FOLDER_OBJECT_PATH "/org/gnome/evolution/camel/folder"
#define CAMEL_STORE_OBJECT_PATH "/org/gnome/evolution/camel/store"

#define CAMEL_SESSION_INTERFACE	"org.gnome.evolution.camel.session.mail"
#define MAIL_SESSION_INTERFACE	"org.gnome.evolution.camel.session.mail"
#define CAMEL_STORE_INTERFACE "org.gnome.evolution.camel.store"
#define CAMEL_FOLDER_INTERFACE "org.gnome.evolution.camel.folder"

typedef struct _CamelHookRemote {
	char *object_id;
	char *signal;
	CamelObjectEventHookFunc func;
	gpointer data;
	guint remote_id;
}CamelHookRemote;

typedef enum {
	CAMEL_RO_SESSION=0,
	CAMEL_RO_MSESSION,
	CAMEL_RO_STORE,
	CAMEL_RO_FOLDER
}CamelRemoteObjectType;


typedef struct _CamelObjectRemote {
	char *object_id;
	CamelRemoteObjectType type;
	GList *hooks; /* Hooks of CamelRemoteHook */
}CamelObjectRemote;


unsigned int
camel_object_remote_hook_event (CamelObjectRemote *object, char *signal, CamelObjectEventHookFunc func, gpointer data);

CamelObjectRemote * camel_object_remote_from_camel_store (CamelStore *store);
CamelStore * camel_object_remote_get_camel_store (CamelObjectRemote *obj);

#endif
