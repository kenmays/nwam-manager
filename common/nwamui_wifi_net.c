/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/* 
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * CDDL HEADER START
 * 
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 * 
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * 
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 * 
 * CDDL HEADER END
 * 
 * File:   nwamui_wifi_net.c
 *
 */

#include <glib-object.h>
#include <glib/gi18n.h>
#include <strings.h>

#include "libnwamui.h"
#include "nwamui_wifi_net.h"

static GObjectClass *parent_class = NULL;


struct _NwamuiWifiNetPrivate {
        gchar                          *essid;            
        nwamui_wifi_security_t          security;
        gchar                          *bssid;            /* May be NULL if unknown */
        nwamui_wifi_signal_strength_t   signal_strength;
        guint                           speed;
        gchar*                          mode;
        nwamui_wifi_wpa_config_t        wpa_config; /* Only valid if security is WPA */
        gchar                          *wep_password;
        gchar                          *wpa_username;
        gchar                          *wpa_password;
        gchar                          *wpa_cert_file;
};

static void nwamui_wifi_net_set_property (  GObject         *object,
                                            guint            prop_id,
                                            const GValue    *value,
                                            GParamSpec      *pspec);

static void nwamui_wifi_net_get_property (  GObject         *object,
                                            guint            prop_id,
                                            GValue          *value,
                                            GParamSpec      *pspec);

static void nwamui_wifi_net_finalize (      NwamuiWifiNet *self);


/* Callbacks */
static void object_notify_cb( GObject *gobject, GParamSpec *arg1, gpointer data);

enum {
	PROP_ESSID = 1,
        PROP_SECURITY,
        PROP_BSSID,
        PROP_SIGNAL_STRENGTH,
        PROP_SPEED,
        PROP_MODE,
        PROP_WPA_CONFIG,
        PROP_WEP_PASSWORD,
        PROP_WPA_USERNAME,
        PROP_WPA_PASSWORD,
        PROP_WPA_CERT_FILE
};

G_DEFINE_TYPE (NwamuiWifiNet, nwamui_wifi_net, G_TYPE_OBJECT)

static void
nwamui_wifi_net_class_init (NwamuiWifiNetClass *klass)
{
    /* Pointer to GObject Part of Class */
    GObjectClass *gobject_class = (GObjectClass*) klass;
        
    /* Initialise Static Parent Class pointer */
    parent_class = g_type_class_peek_parent (klass);

    /* Override Some Function Pointers */
    gobject_class->set_property = nwamui_wifi_net_set_property;
    gobject_class->get_property = nwamui_wifi_net_get_property;
    gobject_class->finalize = (void (*)(GObject*)) nwamui_wifi_net_finalize;

    /* Create some properties */
    g_object_class_install_property (gobject_class,
                                     PROP_ESSID,
                                     g_param_spec_string ("essid",
                                                          _("Wifi Network ESSID"),
                                                          _("Wifi Network ESSID"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_BSSID,
                                     g_param_spec_string ("bssid",
                                                          _("Wifi Network BSSID"),
                                                          _("Wifi Network BSSID"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_SECURITY,
                                     g_param_spec_int    ("security",
                                                          _("Wifi Network Security"),
                                                          _("Wifi Network Security"),
                                                          NWAMUI_WIFI_SEC_NONE, /* Min */
                                                          NWAMUI_WIFI_SEC_LAST-1, /* Max */
                                                          NWAMUI_WIFI_SEC_NONE, /* Default */
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (gobject_class,
                                     PROP_SIGNAL_STRENGTH,
                                     g_param_spec_int    ("signal_strength",
                                                          _("Wifi Network Signal Strength"),
                                                          _("Wifi Network Signal Strength"),
                                                          NWAMUI_WIFI_STRENGTH_NONE, /* Min */
                                                          NWAMUI_WIFI_STRENGTH_EXCELLENT, /* Max */
                                                          NWAMUI_WIFI_STRENGTH_NONE, /* Default */
                                                          G_PARAM_READWRITE));    

    g_object_class_install_property (gobject_class,
                                     PROP_SPEED,
                                     g_param_spec_uint ("speed",
                                                       _("speed"),
                                                       _("speed"),
                                                       0,
                                                       G_MAXUINT,
                                                       0,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_MODE,
                                     g_param_spec_string ("mode",
                                                          _("mode"),
                                                          _("mode"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_WPA_CONFIG,
                                     g_param_spec_int    ("wpa_config",
                                                          _("Wifi WPA Configuration Type"),
                                                          _("Wifi WPA Configuration Type"),
                                                          NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC, /* Min */
                                                          NWAMUI_WIFI_WPA_CONFIG_LAST-1, /* Max */
                                                          NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC, /* Default */
                                                          G_PARAM_READWRITE));


    g_object_class_install_property (gobject_class,
                                     PROP_WEP_PASSWORD,
                                     g_param_spec_string ("wep_password",
                                                          _("Wifi WEP Password"),
                                                          _("Wifi WEP Password"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_WPA_USERNAME,
                                     g_param_spec_string ("wpa_username",
                                                          _("Wifi WPA Username"),
                                                          _("Wifi WPA Username"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_WPA_PASSWORD,
                                     g_param_spec_string ("wpa_password",
                                                          _("Wifi WPA Password"),
                                                          _("Wifi WPA Password"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_WPA_CERT_FILE,
                                     g_param_spec_string ("wpa_cert_file",
                                                          _("Wifi WPA Certificate File Path"),
                                                          _("Wifi WPA Certificate File Path"),
                                                          "",
                                                          G_PARAM_READWRITE));

}


static void
nwamui_wifi_net_init (NwamuiWifiNet *self)
{
    NwamuiWifiNetPrivate *prv = (NwamuiWifiNetPrivate*)g_new0 (NwamuiWifiNetPrivate, 1);

    prv->essid = NULL;            
    prv->security = NWAMUI_WIFI_SEC_NONE;
    prv->bssid = NULL;
    prv->signal_strength = NWAMUI_WIFI_STRENGTH_NONE;
    prv->speed = 0;
    prv->mode = NULL;
    prv->wpa_config = NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC; 
    prv->wep_password = NULL;
    prv->wpa_username = NULL;
    prv->wpa_password = NULL;
    prv->wpa_cert_file = NULL;
    
    self->prv = prv;
    
    g_signal_connect(G_OBJECT(self), "notify", (GCallback)object_notify_cb, (gpointer)self);
}

static void
nwamui_wifi_net_set_property (  GObject         *object,
                                guint            prop_id,
                                const GValue    *value,
                                GParamSpec      *pspec)
{
    NwamuiWifiNet          *self = NWAMUI_WIFI_NET(object);
    NwamuiWifiNetPrivate   *prv;

    g_return_if_fail( self != NULL );
    
    prv = self->prv;
    g_assert( prv != NULL );
    
    switch (prop_id) {
	case PROP_ESSID: {
		const gchar *essid = g_value_get_string (value);
                if ( essid != NULL ) {
                    if ( prv->essid != NULL ) {
                        g_free( prv->essid );
                    }
                    prv->essid = g_strdup(essid);
                }
                else {
                    prv->essid = NULL;
                }
            }
            break;
        case PROP_BSSID: {
		const gchar *bssid = g_value_get_string (value);
                if ( bssid != NULL ) {
                    if ( prv->bssid != NULL ) {
                        g_free( prv->bssid );
                    }
                    prv->bssid = g_strdup(bssid);
                }
                else {
                    prv->bssid = NULL;
                }
            }
            break;
        case PROP_SECURITY: {
                gint tmpint = g_value_get_int (value);

                g_assert( tmpint >= NWAMUI_WIFI_SEC_NONE && tmpint < NWAMUI_WIFI_SEC_LAST );

                prv->security = (nwamui_wifi_security_t)tmpint;
            }
            break;
        case PROP_SIGNAL_STRENGTH: {
                gint tmpint = g_value_get_int (value);

                g_assert( tmpint >= NWAMUI_WIFI_STRENGTH_NONE && tmpint < NWAMUI_WIFI_STRENGTH_LAST );

                prv->signal_strength = (nwamui_wifi_signal_strength_t)tmpint;
            }
            break;

        case PROP_SPEED: {
                self->prv->speed = g_value_get_uint( value );
            }
            break;

        case PROP_MODE: {
                if ( self->prv->mode != NULL ) {
                        g_free( self->prv->mode );
                }
                self->prv->mode = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_WPA_CONFIG: {
                gint tmpint = g_value_get_int (value);

                g_assert( tmpint >= NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC && tmpint < NWAMUI_WIFI_WPA_CONFIG_LAST );

                prv->wpa_config = (nwamui_wifi_wpa_config_t)tmpint;
            }
            break;
        case PROP_WEP_PASSWORD: {
		const gchar *passwd = g_value_get_string (value);
                if ( passwd != NULL ) {
                    if ( prv->wep_password != NULL ) {
                        g_free( prv->wep_password );
                    }
                    prv->wep_password = g_strdup(passwd);
                }
                else {
                    prv->wep_password = NULL;
                }
            }
            break;
        case PROP_WPA_USERNAME: {
		const gchar *username = g_value_get_string (value);
                if ( username != NULL ) {
                    if ( prv->wpa_username != NULL ) {
                        g_free( prv->wpa_username );
                    }
                    prv->wpa_username = g_strdup(username);
                }
                else {
                    prv->wpa_username = NULL;
                }
            }
            break;
        case PROP_WPA_PASSWORD: {
		const gchar *passwd = g_value_get_string (value);
                if ( passwd != NULL ) {
                    if ( prv->wpa_password != NULL ) {
                        g_free( prv->wpa_password );
                    }
                    prv->wpa_password = g_strdup(passwd);
                }
                else {
                    prv->wpa_password = NULL;
                }
            }
            break;
        case PROP_WPA_CERT_FILE: {
		const gchar *fname = g_value_get_string (value);
                if ( fname != NULL ) {
                    if ( prv->wpa_cert_file != NULL ) {
                        g_free( prv->wpa_cert_file );
                    }
                    prv->wpa_cert_file = g_strdup(fname);
                }
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
nwamui_wifi_net_get_property (GObject         *object,
                                  guint            prop_id,
                                  GValue          *value,
                                  GParamSpec      *pspec)
{
    NwamuiWifiNet *self = NWAMUI_WIFI_NET(object);
    NwamuiWifiNetPrivate   *prv;

    g_return_if_fail( self != NULL );
    
    prv = self->prv;
    g_assert( prv != NULL );

    switch (prop_id) {
	case PROP_ESSID: {
                g_value_set_string(value, prv->essid);
            }
            break;
        case PROP_BSSID: {
                g_value_set_string(value, prv->bssid);
            }
            break;
        case PROP_SECURITY: {
                g_value_set_int(value, (gint)prv->security );
            }
            break;
        case PROP_SIGNAL_STRENGTH: {
                g_value_set_int(value, (gint)prv->signal_strength );
            }
            break;
        case PROP_SPEED: {
                g_value_set_uint( value, self->prv->speed );
            }
            break;

        case PROP_MODE: {
                g_value_set_string( value, self->prv->mode );
            }
            break;
        case PROP_WPA_CONFIG: {
                g_value_set_int(value, (gint)prv->wpa_config );
            }
            break;
        case PROP_WEP_PASSWORD: {
                g_value_set_string(value, prv->wep_password);
            }
            break;
        case PROP_WPA_USERNAME: {
                g_value_set_string(value, prv->wpa_username);
            }
            break;
        case PROP_WPA_PASSWORD: {
                g_value_set_string(value, prv->wpa_password);
            }
            break;
        case PROP_WPA_CERT_FILE: {
                g_value_set_string(value, prv->wpa_cert_file);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
nwamui_wifi_net_finalize (NwamuiWifiNet *self)
{
    if ( self->prv ) {
        if ( self->prv->essid ) {
            g_free( self->prv->essid );
        }
        if ( self->prv->bssid ) {
            g_free( self->prv->bssid );
        }
        if (self->prv->mode != NULL ) {
            g_free( self->prv->mode );
        }
        
        g_free (self->prv); 
    }
    self->prv = NULL;

    parent_class->finalize (G_OBJECT (self));
}

/* Exported Functions */

/**
 * nwamui_wifi_net_new:
 *
 * @essid: a C string containing the ESSID of the link
 * @security: a #nwamui_wifi_security_t
 * @bssid: a C string with the BSSID of the AP.
 * @signal_strength: a nwamui_wifi_signal_strength_t
 * @wpa_config: a nwamui_wifi_wpa_config_t, only assumed valid if #security is a WPA type.
 * 
 * @returns: a new #NwamuiWifiNet.
 *
 * Creates a new #NwamuiWifiNet.
 **/
extern  NwamuiWifiNet*
nwamui_wifi_net_new(    const gchar                     *essid, 
                        nwamui_wifi_security_t           security,
                        const gchar                     *bssid,
                        const gchar                     *mode,
                        guint                            speed,
                        nwamui_wifi_signal_strength_t    signal_strength,
                        nwamui_wifi_wpa_config_t         wpa_config )
{
    NwamuiWifiNet*  self = NULL;
    
    self = NWAMUI_WIFI_NET(g_object_new (NWAMUI_TYPE_WIFI_NET, NULL));
    
    g_object_set (G_OBJECT (self),
                    "essid", essid,
                    "security", security,
                    "bssid", bssid,
                    "mode", mode,
                    "speed", speed,
                    "signal_strength", signal_strength,
                    "wpa_config", wpa_config,
                    NULL);
    
    return( self );
}

/* Get/Set ESSID */
void                    
nwamui_wifi_net_set_essid ( NwamuiWifiNet  *self,
                            const gchar    *essid )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (essid != NULL );

    if ( essid != NULL ) {
        g_object_set (G_OBJECT (self),
                      "essid", essid,
                      NULL);
    }
}
                                
gchar*                  
nwamui_wifi_net_get_essid (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "essid", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Get/Set BSSID */
extern void                    
nwamui_wifi_net_set_bssid ( NwamuiWifiNet  *self,
                            const gchar    *bssid )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (bssid != NULL );

    if ( bssid != NULL ) {
        g_object_set (G_OBJECT (self),
                      "bssid", bssid,
                      NULL);
    }
}
                                
extern gchar*                  
nwamui_wifi_net_get_bssid (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "bssid", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Get/Set Security Type */
extern void                    
nwamui_wifi_net_set_security ( NwamuiWifiNet           *self,
                               nwamui_wifi_security_t   security )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (security >= NWAMUI_WIFI_SEC_NONE && security < NWAMUI_WIFI_SEC_LAST );

    g_object_set (G_OBJECT (self),
                  "security", security,
                  NULL);
}
                                
extern nwamui_wifi_security_t           
nwamui_wifi_net_get_security (NwamuiWifiNet *self )
{
    gint    retval = NWAMUI_WIFI_SEC_NONE;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), (nwamui_wifi_security_t)retval);

    g_object_get (G_OBJECT (self),
                  "security", &retval,
                  NULL);
    
    return( (nwamui_wifi_security_t)retval );
}

/* Get/Set Signal Strength */
extern void                    
nwamui_wifi_net_set_signal_strength (   NwamuiWifiNet                  *self,
                                        nwamui_wifi_signal_strength_t   signal_strength )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (signal_strength >= NWAMUI_WIFI_STRENGTH_NONE && signal_strength < NWAMUI_WIFI_STRENGTH_LAST );

    g_object_set (G_OBJECT (self),
                  "signal_strength", signal_strength,
                  NULL);
}
                                
extern nwamui_wifi_signal_strength_t          
nwamui_wifi_net_get_signal_strength (NwamuiWifiNet *self )
{
    gint    retval = NWAMUI_WIFI_SEC_NONE;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), (nwamui_wifi_signal_strength_t)retval);

    g_object_get (G_OBJECT (self),
                  "signal_strength", &retval,
                  NULL);
    
    return( (nwamui_wifi_signal_strength_t)retval );
}

/** 
 * nwamui_wifi_net_set_speed:
 * @nwamui_wifi_net: a #NwamuiWifiNet.
 * @speed: Value to set speed to.
 * 
 **/ 
extern void
nwamui_wifi_net_set_speed (   NwamuiWifiNet *self,
                              guint        speed )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET (self));
    g_assert (speed >= 0 && speed <= G_MAXUINT );

    g_object_set (G_OBJECT (self),
                  "speed", speed,
                  NULL);
}

/**
 * nwamui_wifi_net_get_speed:
 * @nwamui_wifi_net: a #NwamuiWifiNet.
 * @returns: the speed.
 *
 **/
extern guint
nwamui_wifi_net_get_speed (NwamuiWifiNet *self)
{
    guint  speed = 0; 

    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), speed);

    g_object_get (G_OBJECT (self),
                  "speed", &speed,
                  NULL);

    return( speed );
}

/** 
 * nwamui_wifi_net_set_mode:
 * @nwamui_wifi_net: a #NwamuiWifiNet.
 * @mode: Value to set mode to.
 * 
 **/ 
extern void
nwamui_wifi_net_set_mode (   NwamuiWifiNet *self,
                              const gchar*  mode )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET (self));
    g_assert (mode != NULL );

    if ( mode != NULL ) {
        g_object_set (G_OBJECT (self),
                      "mode", mode,
                      NULL);
    }
}

/**
 * nwamui_wifi_net_get_mode:
 * @nwamui_wifi_net: a #NwamuiWifiNet.
 * @returns: the mode.
 *
 **/
extern gchar*
nwamui_wifi_net_get_mode (NwamuiWifiNet *self)
{
    gchar*  mode = NULL; 

    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), mode);

    g_object_get (G_OBJECT (self),
                  "mode", &mode,
                  NULL);

    return( mode );
}

/* Get/Set WPA Configuration Type */
extern void                    
nwamui_wifi_net_set_wpa_config (    NwamuiWifiNet           *self,
                                    nwamui_wifi_wpa_config_t wpa_config )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (wpa_config >= NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC && wpa_config < NWAMUI_WIFI_WPA_CONFIG_LAST );

    g_object_set (G_OBJECT (self),
                  "wpa_config", wpa_config,
                  NULL);
}
                                
extern nwamui_wifi_wpa_config_t
nwamui_wifi_net_get_wpa_config (NwamuiWifiNet *self )
{
    gint    retval = NWAMUI_WIFI_SEC_NONE;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), (nwamui_wifi_wpa_config_t)retval);

    g_object_get (G_OBJECT (self),
                  "wpa_config", &retval,
                  NULL);
    
    return( (nwamui_wifi_wpa_config_t)retval );
}

/* Get/Set WEP Password/Key */
extern void                    
nwamui_wifi_net_set_wep_password (  NwamuiWifiNet  *self,
                                    const gchar    *wep_password )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (wep_password != NULL );

    if ( wep_password != NULL ) {
        g_object_set (G_OBJECT (self),
                      "wep_password", wep_password,
                      NULL);
    }
}
                                
extern gchar*                  
nwamui_wifi_net_get_wep_password (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "wep_password", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Get/Set WPA Username */
extern void                    
nwamui_wifi_net_set_wpa_username (  NwamuiWifiNet  *self,
                                    const gchar    *wpa_username )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (wpa_username != NULL );

    if ( wpa_username != NULL ) {
        g_object_set (G_OBJECT (self),
                      "wpa_username", wpa_username,
                      NULL);
    }
}
                                
extern gchar*                  
nwamui_wifi_net_get_wpa_username (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "wpa_username", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Get/Set WPA Password */
extern void                    
nwamui_wifi_net_set_wpa_password ( NwamuiWifiNet  *self,
                                   const gchar    *wpa_password )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (wpa_password != NULL );

    if ( wpa_password != NULL ) {
        g_object_set (G_OBJECT (self),
                      "wpa_password", wpa_password,
                      NULL);
    }
}
                                
extern gchar*                  
nwamui_wifi_net_get_wpa_password (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "wpa_password", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Get/Set WPA Certificate File */
extern void                    
nwamui_wifi_net_set_wpa_cert_file ( NwamuiWifiNet  *self,
                                    const gchar    *wpa_cert_file )
{
    g_return_if_fail (NWAMUI_IS_WIFI_NET(self));
    g_assert (wpa_cert_file != NULL );

    if ( wpa_cert_file != NULL ) {
        g_object_set (G_OBJECT (self),
                      "wpa_cert_file", wpa_cert_file,
                      NULL);
    }
}
                                
extern gchar*                  
nwamui_wifi_net_get_wpa_cert_file (NwamuiWifiNet *self )
{
    gchar*  ret_str = NULL;
    
    g_return_val_if_fail (NWAMUI_IS_WIFI_NET (self), ret_str);

    g_object_get (G_OBJECT (self),
                  "wpa_cert_file", &ret_str,
                  NULL);
    
    return( ret_str );
}

/* Callbacks */

static void
object_notify_cb( GObject *gobject, GParamSpec *arg1, gpointer data)
{
    NwamuiWifiNet* self = NWAMUI_WIFI_NET(data);

    g_debug("NwamuiWifiNet: notify %s changed\n", arg1->name);
}
