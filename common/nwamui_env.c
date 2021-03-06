/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/* 
 * Copyright 2007-2009 Sun Microsystems, Inc.  All rights reserved.
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
 * File:   nwamui_env.c
 *
 */

#include <libnwam.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>

#include "libnwamui.h"

enum {
    SVC_OBJECT = 0,
    SVC_N_COL,
};

struct _NwamuiEnvPrivate {
    gchar*                      name;
    nwam_loc_handle_t			nwam_loc;
    gboolean                    nwam_loc_modified;
    gboolean                    enabled; /* Cache state we we can "enable" on commit */

    GtkListStore*               svcs_model;
    GtkListStore*               sys_svcs_model;

    /* Not used for Phase 1 any more */
#ifdef ENABLE_PROXY
    nwamui_env_proxy_type_t     proxy_type;
    gboolean                    use_http_proxy_for_all;
    gchar*                      proxy_pac_file;
    gchar*                      proxy_http_server;
    gchar*                      proxy_https_server;
    gchar*                      proxy_ftp_server;
    gchar*                      proxy_gopher_server;
    gchar*                      proxy_socks_server;
    gchar*                      proxy_bypass_list;
    gint                        proxy_http_port;
    gint                        proxy_https_port;
    gint                        proxy_ftp_port;
    gint                        proxy_gopher_port;
    gint                        proxy_socks_port;
    gchar*                      proxy_username;
    gchar*                      proxy_password;
#endif /* ENABLE_PROXY */

};

enum {
    PROP_NAMESERVICES = 1,
    PROP_NAMESERVICES_CONFIG_FILE,
    PROP_DEFAULT_DOMAIN,
    PROP_DNS_NAMESERVICE_CONFIG_SOURCE,
    PROP_DNS_NAMESERVICE_DOMAIN,
    PROP_DNS_NAMESERVICE_SERVERS,
    PROP_DNS_NAMESERVICE_SEARCH,
    PROP_DNS_NAMESERVICE_OPTIONS,
    PROP_DNS_NAMESERVICE_SORTLIST,
    PROP_NIS_NAMESERVICE_CONFIG_SOURCE,
    PROP_NIS_NAMESERVICE_SERVERS,
    PROP_LDAP_NAMESERVICE_CONFIG_SOURCE,
    PROP_LDAP_NAMESERVICE_SERVERS,
#ifndef _DISABLE_HOSTS_FILE
    PROP_HOSTS_FILE,
#endif /* _DISABLE_HOSTS_FILE */
    PROP_NFSV4_DOMAIN,
    PROP_IPFILTER_CONFIG_FILE,
    PROP_IPFILTER_V6_CONFIG_FILE,
    PROP_IPNAT_CONFIG_FILE,
    PROP_IPPOOL_CONFIG_FILE,
    PROP_IKE_CONFIG_FILE,
    PROP_IPSECPOLICY_CONFIG_FILE,
#ifdef ENABLE_NETSERVICES
    PROP_SVCS_ENABLE,
    PROP_SVCS_DISABLE,
#endif /* ENABLE_NETSERVICES */
    PROP_SVCS,
#ifdef ENABLE_PROXY
    PROP_PROXY_TYPE,
    PROP_PROXY_PAC_FILE,
    PROP_USE_HTTP_PROXY_FOR_ALL,
    PROP_PROXY_HTTP_SERVER,
    PROP_PROXY_HTTPS_SERVER,
    PROP_PROXY_FTP_SERVER,
    PROP_PROXY_GOPHER_SERVER,
    PROP_PROXY_SOCKS_SERVER,
    PROP_PROXY_BYPASS_LIST,
    PROP_PROXY_HTTP_PORT,
    PROP_PROXY_HTTPS_PORT,
    PROP_PROXY_FTP_PORT,
    PROP_PROXY_GOPHER_PORT,
    PROP_PROXY_SOCKS_PORT,
    PROP_PROXY_USERNAME,
    PROP_PROXY_PASSWORD
#endif /* ENABLE_PROXY */
};

#define NWAMUI_ENV_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), NWAMUI_TYPE_ENV, NwamuiEnvPrivate))

static void nwamui_env_set_property ( GObject         *object,
                                      guint            prop_id,
                                      const GValue    *value,
                                      GParamSpec      *pspec);

static void nwamui_env_get_property ( GObject         *object,
                                      guint            prop_id,
                                      GValue          *value,
                                      GParamSpec      *pspec);

static void nwamui_env_finalize (     NwamuiEnv *self);

static guint64*     convert_name_services_glist_to_unint64_array( GList* ns_glist, guint *count );
static GList*       convert_name_services_uint64_array_to_glist( guint64* ns_list, guint count );

static gboolean     get_nwam_loc_boolean_prop( nwam_loc_handle_t loc, const char* prop_name );
static gboolean     set_nwam_loc_boolean_prop( nwam_loc_handle_t loc, const char* prop_name, gboolean bool_value );

static gchar*       get_nwam_loc_string_prop( nwam_loc_handle_t loc, const char* prop_name );
static gboolean     set_nwam_loc_string_prop( nwam_loc_handle_t loc, const char* prop_name, const char* str );

static gchar**      get_nwam_loc_string_array_prop( nwam_loc_handle_t loc, const char* prop_name );
static gboolean     set_nwam_loc_string_array_prop( nwam_loc_handle_t loc, const char* prop_name, char** strs, guint len);

static guint64      get_nwam_loc_uint64_prop( nwam_loc_handle_t loc, const char* prop_name );
static gboolean     set_nwam_loc_uint64_prop( nwam_loc_handle_t loc, const char* prop_name, guint64 value );

static guint64*     get_nwam_loc_uint64_array_prop( nwam_loc_handle_t loc, const char* prop_name , guint* out_num );
static gboolean     set_nwam_loc_uint64_array_prop( nwam_loc_handle_t loc, const char* prop_name , 
                                                    const guint64* value, guint len );

static gint         nwamui_object_real_open(NwamuiObject *object, const gchar *name, gint flag);
static nwam_state_t nwamui_object_real_get_nwam_state(NwamuiObject *object, nwam_aux_state_t* aux_state_p, const gchar**aux_state_string_p);
static gboolean     nwamui_object_real_can_rename (NwamuiObject *object);
static gboolean     nwamui_object_real_set_name ( NwamuiObject *object, const gchar* name );
static const gchar* nwamui_object_real_get_name ( NwamuiObject *object );
static void         nwamui_object_real_set_activation_mode ( NwamuiObject *object, gint activation_mode );
static gint         nwamui_object_real_get_activation_mode ( NwamuiObject *object );
static void         nwamui_object_real_set_conditions ( NwamuiObject *object, const GList* conditions );
static GList*       nwamui_object_real_get_conditions ( NwamuiObject *object );
static gboolean     nwamui_object_real_get_active (NwamuiObject *object);
static void         nwamui_object_real_set_active (NwamuiObject *object, gboolean active );
static void         nwamui_object_real_set_enabled ( NwamuiObject *object, gboolean enabled );
static gboolean     nwamui_object_real_get_enabled ( NwamuiObject *object );
static gboolean     nwamui_object_real_validate(NwamuiObject *object, gchar **prop_name_ret);
static gboolean     nwamui_object_real_commit( NwamuiObject* object );
static gboolean     nwamui_object_real_destroy( NwamuiObject* object );
static gboolean     nwamui_object_real_is_modifiable(NwamuiObject *object);
static void         nwamui_object_real_reload(NwamuiObject* object);
static NwamuiObject* nwamui_object_real_clone(NwamuiObject *object, const gchar *name, NwamuiObject *parent);
static gboolean     nwamui_object_real_has_modifications(NwamuiObject* object);

#if 0
/* These are not needed right now since we don't support property templates,
 * but would like to keep around for when we do.
 */
static gboolean nwamui_env_svc_commit (NwamuiEnv *self, NwamuiSvc *svc);
#endif /* 0 */

/* Callbacks */
static void svc_row_inserted_or_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data); 
static void svc_row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path, gpointer user_data);

#if 0
/* These are not needed right now since we don't support property templates,
 * but would like to keep around for when we do.
 */
/* walkers */
static int nwam_loc_svc_walker_cb (nwam_loc_prop_template_t svc, void *data);
static int nwam_loc_sys_svc_walker_cb (nwam_loc_handle_t env, nwam_loc_prop_template_t svc, void *data);
#endif /* 0 */

G_DEFINE_TYPE (NwamuiEnv, nwamui_env, NWAMUI_TYPE_OBJECT)

static void
nwamui_env_class_init (NwamuiEnvClass *klass)
{
    /* Pointer to GObject Part of Class */
    GObjectClass *gobject_class = (GObjectClass*) klass;
    NwamuiObjectClass *nwamuiobject_class = NWAMUI_OBJECT_CLASS(klass);
        
    /* Override Some Function Pointers */
    gobject_class->set_property = nwamui_env_set_property;
    gobject_class->get_property = nwamui_env_get_property;
    gobject_class->finalize = (void (*)(GObject*)) nwamui_env_finalize;

    nwamuiobject_class->open = nwamui_object_real_open;
    nwamuiobject_class->get_name = nwamui_object_real_get_name;
    nwamuiobject_class->can_rename = nwamui_object_real_can_rename;
    nwamuiobject_class->set_name = nwamui_object_real_set_name;
    nwamuiobject_class->get_conditions = nwamui_object_real_get_conditions;
    nwamuiobject_class->set_conditions = nwamui_object_real_set_conditions;
    nwamuiobject_class->get_activation_mode = nwamui_object_real_get_activation_mode;
    nwamuiobject_class->set_activation_mode = nwamui_object_real_set_activation_mode;
    nwamuiobject_class->get_active = nwamui_object_real_get_active;
    nwamuiobject_class->set_active = nwamui_object_real_set_active;
    nwamuiobject_class->get_enabled = nwamui_object_real_get_enabled;
    nwamuiobject_class->set_enabled = nwamui_object_real_set_enabled;
    nwamuiobject_class->get_nwam_state = nwamui_object_real_get_nwam_state;
    nwamuiobject_class->validate = nwamui_object_real_validate;
    nwamuiobject_class->commit = nwamui_object_real_commit;
    nwamuiobject_class->reload = nwamui_object_real_reload;
    nwamuiobject_class->destroy = nwamui_object_real_destroy;
    nwamuiobject_class->is_modifiable = nwamui_object_real_is_modifiable;
    nwamuiobject_class->clone = nwamui_object_real_clone;
    nwamuiobject_class->has_modifications = nwamui_object_real_has_modifications;

	g_type_class_add_private(klass, sizeof(NwamuiEnvPrivate));

    /* Create some properties */
    g_object_class_install_property (gobject_class,
                                     PROP_NAMESERVICES,
                                     g_param_spec_pointer ("nameservices",
                                                          _("nameservices"),
                                                          _("nameservices"),
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_NAMESERVICES_CONFIG_FILE,
                                     g_param_spec_string ("nameservices_config_file",
                                                          _("nameservices_config_file"),
                                                          _("nameservices_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DEFAULT_DOMAIN,
                                     g_param_spec_string ("default_domainname",
                                                          _("default_domainname"),
                                                          _("default_domainname"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DNS_NAMESERVICE_DOMAIN,
                                     g_param_spec_string ("dns_nameservice_domain",
                                                          _("dns_nameservice_domain"),
                                                          _("dns_nameservice_domain"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DNS_NAMESERVICE_CONFIG_SOURCE,
                                     g_param_spec_int ("dns_nameservice_config_source",
                                                       _("dns_nameservice_config_source"),
                                                       _("dns_nameservice_config_source"),
                                                       NWAMUI_ENV_CONFIG_SOURCE_MANUAL,
                                                       NWAMUI_ENV_CONFIG_SOURCE_LAST-1,
                                                       NWAMUI_ENV_CONFIG_SOURCE_DHCP,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DNS_NAMESERVICE_SERVERS,
                                     g_param_spec_pointer ("dns_nameservice_servers",
                                                          _("dns_nameservice_servers"),
                                                          _("dns_nameservice_servers"),
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DNS_NAMESERVICE_SEARCH,
                                     g_param_spec_pointer ("dns_nameservice_search",
                                                          _("dns_nameservice_search"),
                                                          _("dns_nameservice_search"),
                                                          G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class,
      PROP_DNS_NAMESERVICE_OPTIONS,
      g_param_spec_string("dns_nameservice_options",
        _("dns_nameservice_options"),
        _("dns_nameservice_options"),
        "",
        G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class,
      PROP_DNS_NAMESERVICE_SORTLIST,
      g_param_spec_pointer("dns_nameservice_sortlist",
        _("dns_nameservice_sortlist"),
        _("dns_nameservice_sortlist"),
        G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_NIS_NAMESERVICE_CONFIG_SOURCE,
                                     g_param_spec_int ("nis_nameservice_config_source",
                                                       _("nis_nameservice_config_source"),
                                                       _("nis_nameservice_config_source"),
                                                       NWAMUI_ENV_CONFIG_SOURCE_MANUAL,
                                                       NWAMUI_ENV_CONFIG_SOURCE_LAST-1,
                                                       NWAMUI_ENV_CONFIG_SOURCE_DHCP,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_NIS_NAMESERVICE_SERVERS,
                                     g_param_spec_pointer ("nis_nameservice_servers",
                                                          _("nis_nameservice_servers"),
                                                          _("nis_nameservice_servers"),
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_LDAP_NAMESERVICE_CONFIG_SOURCE,
                                     g_param_spec_int ("ldap_nameservice_config_source",
                                                       _("ldap_nameservice_config_source"),
                                                       _("ldap_nameservice_config_source"),
                                                       NWAMUI_ENV_CONFIG_SOURCE_MANUAL,
                                                       NWAMUI_ENV_CONFIG_SOURCE_LAST-1,
                                                       NWAMUI_ENV_CONFIG_SOURCE_DHCP,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_LDAP_NAMESERVICE_SERVERS,
                                     g_param_spec_pointer ("ldap_nameservice_servers",
                                                          _("ldap_nameservice_servers"),
                                                          _("ldap_nameservice_servers"),
                                                          G_PARAM_READWRITE));

#ifndef _DISABLE_HOSTS_FILE
    g_object_class_install_property (gobject_class,
                                     PROP_HOSTS_FILE,
                                     g_param_spec_string ("hosts_file",
                                                          _("hosts_file"),
                                                          _("hosts_file"),
                                                          "",
                                                          G_PARAM_READWRITE));
#endif /* _DISABLE_HOSTS_FILE */

    g_object_class_install_property (gobject_class,
                                     PROP_NFSV4_DOMAIN,
                                     g_param_spec_string ("nfsv4_domain",
                                                          _("nfsv4_domain"),
                                                          _("nfsv4_domain"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IPFILTER_CONFIG_FILE,
                                     g_param_spec_string ("ipfilter_config_file",
                                                          _("ipfilter_config_file"),
                                                          _("ipfilter_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IPFILTER_V6_CONFIG_FILE,
                                     g_param_spec_string ("ipfilter_v6_config_file",
                                                          _("ipfilter_v6_config_file"),
                                                          _("ipfilter_v6_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IPNAT_CONFIG_FILE,
                                     g_param_spec_string ("ipnat_config_file",
                                                          _("ipnat_config_file"),
                                                          _("ipnat_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IPPOOL_CONFIG_FILE,
                                     g_param_spec_string ("ippool_config_file",
                                                          _("ippool_config_file"),
                                                          _("ippool_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IKE_CONFIG_FILE,
                                     g_param_spec_string ("ike_config_file",
                                                          _("ike_config_file"),
                                                          _("ike_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_IPSECPOLICY_CONFIG_FILE,
                                     g_param_spec_string ("ipsecpolicy_config_file",
                                                          _("ipsecpolicy_config_file"),
                                                          _("ipsecpolicy_config_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

#ifdef ENABLE_NETSERVICES
    g_object_class_install_property (gobject_class,
                                     PROP_SVCS_ENABLE,
                                     g_param_spec_pointer ("svcs_enable",
                                                          _("svcs_enable"),
                                                          _("svcs_enable"),
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_SVCS_DISABLE,
                                     g_param_spec_pointer ("svcs_disable",
                                                          _("svcs_disable"),
                                                          _("svcs_disable"),
                                                          G_PARAM_READWRITE));
#endif /* ENABLE_NETSERVICES */

    g_object_class_install_property (gobject_class,
                                     PROP_SVCS,
                                     g_param_spec_object ("svcs",
                                                          _("smf services"),
                                                          _("smf services"),
                                                          GTK_TYPE_TREE_MODEL,
                                                          G_PARAM_READABLE));
    
#ifdef ENABLE_PROXY
    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_TYPE,
                                     g_param_spec_int ("proxy_type",
                                                       _("proxy_type"),
                                                       _("proxy_type"),
                                                       NWAMUI_ENV_PROXY_TYPE_DIRECT,
                                                       NWAMUI_ENV_PROXY_TYPE_LAST-1,
                                                       NWAMUI_ENV_PROXY_TYPE_DIRECT,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_PAC_FILE,
                                     g_param_spec_string ("proxy_pac_file",
                                                          _("proxy_pac_file"),
                                                          _("proxy_pac_file"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_USE_HTTP_PROXY_FOR_ALL,
                                     g_param_spec_boolean ("use_http_proxy_for_all",
                                                          _("use_http_proxy_for_all"),
                                                          _("use_http_proxy_for_all"),
                                                          FALSE,
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_HTTP_SERVER,
                                     g_param_spec_string ("proxy_http_server",
                                                          _("proxy_http_server"),
                                                          _("proxy_http_server"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_HTTPS_SERVER,
                                     g_param_spec_string ("proxy_https_server",
                                                          _("proxy_https_server"),
                                                          _("proxy_https_server"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_FTP_SERVER,
                                     g_param_spec_string ("proxy_ftp_server",
                                                          _("proxy_ftp_server"),
                                                          _("proxy_ftp_server"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_GOPHER_SERVER,
                                     g_param_spec_string ("proxy_gopher_server",
                                                          _("proxy_gopher_server"),
                                                          _("proxy_gopher_server"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_SOCKS_SERVER,
                                     g_param_spec_string ("proxy_socks_server",
                                                          _("proxy_socks_server"),
                                                          _("proxy_socks_server"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_BYPASS_LIST,
                                     g_param_spec_string ("proxy_bypass_list",
                                                          _("proxy_bypass_list"),
                                                          _("proxy_bypass_list"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_HTTP_PORT,
                                     g_param_spec_int ("proxy_http_port",
                                                       _("proxy_http_port"),
                                                       _("proxy_http_port"),
                                                       0,
                                                       G_MAXINT,
                                                       80,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_HTTPS_PORT,
                                     g_param_spec_int ("proxy_https_port",
                                                       _("proxy_https_port"),
                                                       _("proxy_https_port"),
                                                       0,
                                                       G_MAXINT,
                                                       80,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_FTP_PORT,
                                     g_param_spec_int ("proxy_ftp_port",
                                                       _("proxy_ftp_port"),
                                                       _("proxy_ftp_port"),
                                                       0,
                                                       G_MAXINT,
                                                       80,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_GOPHER_PORT,
                                     g_param_spec_int ("proxy_gopher_port",
                                                       _("proxy_gopher_port"),
                                                       _("proxy_gopher_port"),
                                                       0,
                                                       G_MAXINT,
                                                       80,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_SOCKS_PORT,
                                     g_param_spec_int ("proxy_socks_port",
                                                       _("proxy_socks_port"),
                                                       _("proxy_socks_port"),
                                                       0,
                                                       G_MAXINT,
                                                       1080,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_USERNAME,
                                     g_param_spec_string ("proxy_username",
                                                          _("proxy_username"),
                                                          _("proxy_username"),
                                                          "",
                                                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_PASSWORD,
                                     g_param_spec_string ("proxy_password",
                                                          _("proxy_password"),
                                                          _("proxy_password"),
                                                          "",
                                                          G_PARAM_READWRITE));
#endif /* ENABLE_PROXY */
}


static void
nwamui_env_init (NwamuiEnv *self)
{
    NwamuiEnvPrivate *prv      = NWAMUI_ENV_GET_PRIVATE(self);
    self->prv = prv;
    
    prv->svcs_model = gtk_list_store_new(SVC_N_COL, G_TYPE_OBJECT);

#ifdef ENABLE_PROXY
    prv->proxy_type = NWAMUI_ENV_PROXY_TYPE_DIRECT;
    prv->proxy_http_port = 80;
    prv->proxy_https_port = 80;
    prv->proxy_ftp_port = 80;
    prv->proxy_gopher_port = 80;
    prv->proxy_socks_port = 1080;
#endif /* ENABLE_PROXY */

    /*
    g_signal_connect(G_OBJECT(prv->svcs_model), "row-deleted", (GCallback)svc_row_deleted_cb, (gpointer)self);
    g_signal_connect(G_OBJECT(prv->svcs_model), "row-changed", (GCallback)svc_row_inserted_or_changed_cb, (gpointer)self);
    g_signal_connect(G_OBJECT(prv->svcs_model), "row-inserted", (GCallback)svc_row_inserted_or_changed_cb, (gpointer)self);
    */
}

static void
nwamui_env_set_property (   GObject         *object,
  guint            prop_id,
  const GValue    *value,
  GParamSpec      *pspec)
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    NwamuiEnvPrivate *prv = NWAMUI_ENV(object)->prv;
    gchar*      tmpstr = NULL;
    gint        tmpint = 0;
    nwam_error_t nerr;
    
    switch (prop_id) {
    case PROP_NAMESERVICES:
        nwamui_env_set_nameservices(self, (GList *)g_value_get_pointer(value));
        break;

    case PROP_NAMESERVICES_CONFIG_FILE:
        nwamui_env_set_nameservices_config_file(self, g_value_get_string(value));
        break;

    case PROP_DEFAULT_DOMAIN:
        nwamui_env_set_default_domainname(self, g_value_get_string(value));
        break;

    case PROP_DNS_NAMESERVICE_DOMAIN:
        nwamui_env_set_dns_nameservice_domain(self, g_value_get_string(value));
        break;

    case PROP_DNS_NAMESERVICE_CONFIG_SOURCE:
        nwamui_env_set_dns_nameservice_config_source(self, g_value_get_int(value));
        break;

    case PROP_DNS_NAMESERVICE_SERVERS:
        nwamui_env_set_dns_nameservice_servers(self, (GList *)g_value_get_pointer(value));
        break;

    case PROP_DNS_NAMESERVICE_SEARCH:
        nwamui_env_set_dns_nameservice_search(self, (GList *)g_value_get_pointer(value));
        break;

    case PROP_DNS_NAMESERVICE_OPTIONS:
        nwamui_env_set_dns_nameservice_options(self, g_value_get_string(value));
        break;

    case PROP_DNS_NAMESERVICE_SORTLIST:
        nwamui_env_set_dns_nameservice_sortlist(self, (GList *)g_value_get_pointer(value));
        break;

    case PROP_NIS_NAMESERVICE_CONFIG_SOURCE:
        nwamui_env_set_nis_nameservice_config_source(self, g_value_get_int(value));
        break;

    case PROP_NIS_NAMESERVICE_SERVERS:
        nwamui_env_set_nis_nameservice_servers(self, (GList *)g_value_get_pointer(value));
        break;

    case PROP_LDAP_NAMESERVICE_CONFIG_SOURCE:
        nwamui_env_set_ldap_nameservice_config_source(self, g_value_get_int(value) );
        break;

    case PROP_LDAP_NAMESERVICE_SERVERS:
        nwamui_env_set_ldap_nameservice_servers(self, (GList *)g_value_get_pointer(value));
        break;

#ifndef _DISABLE_HOSTS_FILE
    case PROP_HOSTS_FILE:
        nwamui_env_set_hosts_file(self, g_value_get_string(value));
        break;
#endif /* _DISABLE_HOSTS_FILE */

    case PROP_NFSV4_DOMAIN:
        nwamui_env_set_nfsv4_domain(self, g_value_get_string(value));
        break;

    case PROP_IPFILTER_CONFIG_FILE:
        nwamui_env_set_ipfilter_config_file(self, g_value_get_string(value));
        break;

    case PROP_IPFILTER_V6_CONFIG_FILE:
        nwamui_env_set_ipfilter_v6_config_file(self, g_value_get_string(value));
        break;

    case PROP_IPNAT_CONFIG_FILE:
        nwamui_env_set_ipnat_config_file(self, g_value_get_string(value));
        break;

    case PROP_IPPOOL_CONFIG_FILE:
        nwamui_env_set_ippool_config_file(self, g_value_get_string(value));
        break;

    case PROP_IKE_CONFIG_FILE:
        nwamui_env_set_ike_config_file(self, g_value_get_string(value));
        break;

    case PROP_IPSECPOLICY_CONFIG_FILE:
        nwamui_env_set_ipsecpolicy_config_file(self,g_value_get_string(value));
        break;

#ifdef ENABLE_NETSERVICES
    case PROP_SVCS_ENABLE: {
        GList*  fmri = g_value_get_pointer( value );
        gchar** fmri_strs = nwamui_util_glist_to_strv( fmri );
        set_nwam_loc_string_array_prop( self->prv->nwam_loc, NWAM_LOC_PROP_SVCS_ENABLE, fmri_strs, 0 );
        g_strfreev(fmri_strs);
    }
        break;

    case PROP_SVCS_DISABLE: {
        GList*  fmri = g_value_get_pointer( value );
        gchar** fmri_strs = nwamui_util_glist_to_strv( fmri );
        set_nwam_loc_string_array_prop( self->prv->nwam_loc, NWAM_LOC_PROP_SVCS_DISABLE, fmri_strs, 0 );
        g_strfreev(fmri_strs);
    }
        break;
#endif /* ENABLE_NETSERVICES */

#ifdef ENABLE_PROXY
        case PROP_PROXY_TYPE: {
                self->prv->proxy_type = (nwamui_env_proxy_type_t)g_value_get_int( value );
            }
            break;

        case PROP_PROXY_PAC_FILE: {
                if ( self->prv->proxy_pac_file != NULL ) {
                        g_free( self->prv->proxy_pac_file );
                }
                self->prv->proxy_pac_file = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_USE_HTTP_PROXY_FOR_ALL: {
                self->prv->use_http_proxy_for_all = g_value_get_boolean( value );
            }
            break;

        case PROP_PROXY_HTTP_SERVER: {
                if ( self->prv->proxy_http_server != NULL ) {
                        g_free( self->prv->proxy_http_server );
                }
                self->prv->proxy_http_server = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_HTTPS_SERVER: {
                if ( self->prv->proxy_https_server != NULL ) {
                        g_free( self->prv->proxy_https_server );
                }
                self->prv->proxy_https_server = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_FTP_SERVER: {
                if ( self->prv->proxy_ftp_server != NULL ) {
                        g_free( self->prv->proxy_ftp_server );
                }
                self->prv->proxy_ftp_server = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_GOPHER_SERVER: {
                if ( self->prv->proxy_gopher_server != NULL ) {
                        g_free( self->prv->proxy_gopher_server );
                }
                self->prv->proxy_gopher_server = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_SOCKS_SERVER: {
                if ( self->prv->proxy_socks_server != NULL ) {
                        g_free( self->prv->proxy_socks_server );
                }
                self->prv->proxy_socks_server = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_BYPASS_LIST: {
                if ( self->prv->proxy_bypass_list != NULL ) {
                        g_free( self->prv->proxy_bypass_list );
                }
                self->prv->proxy_bypass_list = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_HTTP_PORT: {
                self->prv->proxy_http_port = g_value_get_int( value );
            }
            break;

        case PROP_PROXY_HTTPS_PORT: {
                self->prv->proxy_https_port = g_value_get_int( value );
            }
            break;

        case PROP_PROXY_FTP_PORT: {
                self->prv->proxy_ftp_port = g_value_get_int( value );
            }
            break;

        case PROP_PROXY_GOPHER_PORT: {
                self->prv->proxy_gopher_port = g_value_get_int( value );
            }
            break;

        case PROP_PROXY_SOCKS_PORT: {
                g_debug("socks_port = %d", self->prv->proxy_socks_port );
                self->prv->proxy_socks_port = g_value_get_int( value );
            }
            break;

        case PROP_PROXY_USERNAME: {
                if ( self->prv->proxy_username != NULL ) {
                        g_free( self->prv->proxy_username );
                }
                self->prv->proxy_username = g_strdup( g_value_get_string( value ) );
            }
            break;

        case PROP_PROXY_PASSWORD: {
                if ( self->prv->proxy_password != NULL ) {
                        g_free( self->prv->proxy_password );
                }
                self->prv->proxy_password = g_strdup( g_value_get_string( value ) );
            }
            break;
#endif /* ENABLE_PROXY */

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

    prv->nwam_loc_modified = TRUE;
}

static void
nwamui_env_get_property (GObject         *object,
  guint            prop_id,
  GValue          *value,
  GParamSpec      *pspec)
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    NwamuiEnvPrivate *prv = NWAMUI_ENV(object)->prv;
    nwam_error_t nerr;
    int num;
    nwam_value_t **nwamdata;

    switch (prop_id) {
    case PROP_NAMESERVICES:
        g_value_set_pointer( value, nwamui_env_get_nameservices(self));
        break;

    case PROP_NAMESERVICES_CONFIG_FILE: {
        gchar* str = nwamui_env_get_nameservices_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_DEFAULT_DOMAIN: {
        gchar* str = nwamui_env_get_default_domainname(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_DNS_NAMESERVICE_DOMAIN: {
        gchar* str = nwamui_env_get_dns_nameservice_domain(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_DNS_NAMESERVICE_CONFIG_SOURCE:
        g_value_set_int( value, 
          (gint)nwamui_env_get_dns_nameservice_config_source(self));
        break;
            
    case PROP_DNS_NAMESERVICE_SERVERS:
        g_value_set_pointer( value, nwamui_env_get_dns_nameservice_servers(self));
        break;

    case PROP_DNS_NAMESERVICE_SEARCH:
        /* We may need to/from convert to , separated string?? */
        g_value_set_pointer( value, nwamui_env_get_dns_nameservice_search(self));
        break;

    case PROP_DNS_NAMESERVICE_OPTIONS:
        g_value_set_string(value, nwamui_env_get_dns_nameservice_options(self));
        break;

    case PROP_DNS_NAMESERVICE_SORTLIST:
        g_value_set_pointer( value, nwamui_env_get_dns_nameservice_sortlist(self));
        break;

    case PROP_NIS_NAMESERVICE_CONFIG_SOURCE:
        g_value_set_int( value, 
          (gint)nwamui_env_get_nis_nameservice_config_source(self));
        break;
            
    case PROP_NIS_NAMESERVICE_SERVERS:
        g_value_set_pointer( value, nwamui_env_get_nis_nameservice_servers(self));
        break;

    case PROP_LDAP_NAMESERVICE_CONFIG_SOURCE:
        g_value_set_int( value, 
          (gint)nwamui_env_get_ldap_nameservice_config_source(self));
        break;
            
    case PROP_LDAP_NAMESERVICE_SERVERS:
        g_value_set_pointer( value, nwamui_env_get_ldap_nameservice_servers(self));
        break;

#ifndef _DISABLE_HOSTS_FILE
    case PROP_HOSTS_FILE: {
        gchar* str = nwamui_env_get_hosts_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;
#endif /* _DISABLE_HOSTS_FILE */

    case PROP_NFSV4_DOMAIN: {
        gchar* str = nwamui_env_get_nfsv4_domain(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IPFILTER_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ipfilter_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IPFILTER_V6_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ipfilter_v6_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IPNAT_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ipnat_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IPPOOL_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ippool_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IKE_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ike_config_file(self);
        g_value_set_string( value, str );
        g_free(str);
    }
        break;

    case PROP_IPSECPOLICY_CONFIG_FILE: {
        gchar* str = nwamui_env_get_ipsecpolicy_config_file(self);
        g_value_set_string( value,  str);
        g_free(str);
    }
        break;

#ifdef ENABLE_NETSERVICES
    case PROP_SVCS_ENABLE: {
        gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_SVCS_ENABLE );
        g_value_set_pointer( value, nwamui_util_strv_to_glist( strv ) );
        g_strfreev( strv );
    }
        break;

    case PROP_SVCS_DISABLE: {
        gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_SVCS_DISABLE );
        g_value_set_pointer( value, nwamui_util_strv_to_glist( strv ) );
        g_strfreev( strv );
    }
        break;
#endif /* ENABLE_NETSERVICES */

        case PROP_SVCS: {
                g_value_set_object (value, self->prv->svcs_model);
            }
            break;

#ifdef ENABLE_PROXY
        case PROP_PROXY_TYPE: {
                g_value_set_int( value, (gint)self->prv->proxy_type );
            }
            break;

        case PROP_PROXY_PAC_FILE: {
                g_value_set_string( value, self->prv->proxy_pac_file );
            }
            break;

        case PROP_USE_HTTP_PROXY_FOR_ALL: {
                g_value_set_boolean( value, self->prv->use_http_proxy_for_all );
            }
            break;

        case PROP_PROXY_HTTP_SERVER: {
                g_value_set_string( value, self->prv->proxy_http_server );
            }
            break;

        case PROP_PROXY_HTTPS_SERVER: {
                g_value_set_string( value, self->prv->proxy_https_server );
            }
            break;

        case PROP_PROXY_FTP_SERVER: {
                g_value_set_string( value, self->prv->proxy_ftp_server );
            }
            break;

        case PROP_PROXY_GOPHER_SERVER: {
                g_value_set_string( value, self->prv->proxy_gopher_server );
            }
            break;

        case PROP_PROXY_SOCKS_SERVER: {
                g_value_set_string( value, self->prv->proxy_socks_server );
            }
            break;

        case PROP_PROXY_BYPASS_LIST: {
                g_value_set_string( value, self->prv->proxy_bypass_list );
            }
            break;

        case PROP_PROXY_HTTP_PORT: {
                g_value_set_int( value, self->prv->proxy_http_port );
            }
            break;

        case PROP_PROXY_HTTPS_PORT: {
                g_value_set_int( value, self->prv->proxy_https_port );
            }
            break;

        case PROP_PROXY_FTP_PORT: {
                g_value_set_int( value, self->prv->proxy_ftp_port );
            }
            break;

        case PROP_PROXY_GOPHER_PORT: {
                g_value_set_int( value, self->prv->proxy_gopher_port );
            }
            break;

        case PROP_PROXY_SOCKS_PORT: {
                g_value_set_int( value, self->prv->proxy_socks_port );
            }
            break;

        case PROP_PROXY_USERNAME: {
                g_value_set_string( value, self->prv->proxy_username );
            }
            break;

        case PROP_PROXY_PASSWORD: {
                g_value_set_string( value, self->prv->proxy_password );
            }
            break;
#endif /* ENABLE_PROXY */
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/**
 * nwamui_env_new:
 * @returns: a new #NwamuiEnv.
 *
 * Creates a new #NwamuiEnv.
 **/
NwamuiObject*
nwamui_env_new ( const gchar* name )
{
    NwamuiObject *self = NWAMUI_OBJECT(g_object_new(NWAMUI_TYPE_ENV, NULL));
    
    nwamui_object_set_name(NWAMUI_OBJECT(self), name);

    if (nwamui_object_real_open(self, name, NWAMUI_OBJECT_OPEN) != 0) {
        nwamui_object_real_open(self, name, NWAMUI_OBJECT_CREATE);
    }

    return self;
}

/**
 * nwamui_env_new_with_handle:
 * @returns: a new #NwamuiEnv.
 *
 * Creates a new #NwamuiEnv.
 **/
NwamuiObject*
nwamui_env_new_with_handle (nwam_loc_handle_t envh)
{
    nwam_error_t  nerr;
    char         *name   = NULL;
    NwamuiObject *object = NULL;

    if ((nerr = nwam_loc_get_name(envh, &name)) != NWAM_SUCCESS) {
        g_warning ("Failed to get name for enm, error: %s", nwam_strerror (nerr));
        return NULL;
    }

    object = g_object_new(NWAMUI_TYPE_ENV, NULL);
    g_assert(NWAMUI_IS_ENV(object));

    /* Will update handle. */
    nwamui_object_set_name(object, name);

    nwamui_object_real_open(object, name, NWAMUI_OBJECT_OPEN);

    nwamui_object_real_reload(object);

    free (name);

#if 0
/* These are not needed right now since we don't support property templates,
 * but would like to keep around for when we do.
 */
    nerr = nwam_walk_loc_prop_templates(nwam_loc_svc_walker_cb, env, 0, &rval );
    if (nerr != NWAM_SUCCESS) {
        g_debug ("[libnwam] nwamui_env_new_with_handle walk svc %s", nwam_strerror (nerr));
    }
#endif /* 0 */
    
    return object;
}

static gchar*
get_nwam_loc_string_prop( nwam_loc_handle_t loc, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gchar*              retval = NULL;
    char*               value = NULL;

    g_return_val_if_fail( prop_name != NULL, retval );

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_loc_get_prop_value (loc, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( (nerr = nwam_value_get_string(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get string value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( value != NULL ) {
        retval  = g_strdup ( value );
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gboolean
set_nwam_loc_string_prop( nwam_loc_handle_t loc, const char* prop_name, const gchar* str )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( loc == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }
    if (str != NULL && *str != '\0') {
        /* str is meaningful */

        if ( (nerr = nwam_value_create_string( (char*)str, &nwam_data )) != NWAM_SUCCESS ) {
            g_debug("Error creating a string value for string %s", str?str:"NULL");
            return retval;
        }

        if ( (nerr = nwam_loc_set_prop_value (loc, prop_name, nwam_data)) != NWAM_SUCCESS ) {
            g_debug("Unable to set value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        }
        else {
            retval = TRUE;
        }

        nwam_value_free(nwam_data);
    } else {
        /* str is empty or NULL */

        if ( (nerr = nwam_loc_delete_prop (loc, prop_name)) != NWAM_SUCCESS ) {
            g_debug("Unable to delete loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        }
        else {
            retval = TRUE;
        }
    }
    
    return( retval );
}


static gchar**
get_nwam_loc_string_array_prop( nwam_loc_handle_t loc, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gchar**             retval = NULL;
    char**              value = NULL;
    uint_t              num = 0;

    g_return_val_if_fail( prop_name != NULL, retval );

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_loc_get_prop_value (loc, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( (nerr = nwam_value_get_string_array(nwam_data, &value, &num )) != NWAM_SUCCESS ) {
        g_debug("Unable to get string value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( value != NULL && num > 0 ) {
        /* Create a NULL terminated list of stirngs, allocate 1 extra place
         * for NULL termination. */
        retval = (gchar**)g_malloc0( sizeof(gchar*) * (num+1) );

        for (int i = 0; i < num; i++ ) {
            retval[i]  = g_strdup ( value[i] );
        }
        retval[num]=NULL;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gboolean
set_nwam_loc_string_array_prop( nwam_loc_handle_t loc, const char* prop_name, char** strs, guint len )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;

    g_return_val_if_fail( prop_name != NULL, retval );
    g_return_val_if_fail(len >= 0, retval);

    if ( loc == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    /* Assume a strv, i.e. NULL terminated list, otherwise strs would be NULL */
    if ( len == 0 && strs != NULL) {
        len = g_strv_length(strs);
    }

    if (strs == NULL || len == 0) {
        nerr = nwam_loc_delete_prop (loc, prop_name);
        if ( nerr != NWAM_SUCCESS || nerr != NWAM_ENTITY_NOT_FOUND) {
            g_debug("Unable to delete loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        }
        else {
            retval = TRUE;
        }
        return retval;
    }

    nerr = nwam_value_create_string_array (strs, len, &nwam_data );
    if (nerr != NWAM_SUCCESS) {
        g_debug("Error creating a value for string array 0x%08X", strs);
        return retval;
    }

    if ( (nerr = nwam_loc_set_prop_value (loc, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gboolean
get_nwam_loc_boolean_prop( nwam_loc_handle_t loc, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    boolean_t           value = FALSE;

    g_return_val_if_fail( prop_name != NULL, value );

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_BOOLEAN ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return value;
    }

    if ( (nerr = nwam_loc_get_prop_value (loc, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    if ( (nerr = nwam_value_get_boolean(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get boolean value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    nwam_value_free(nwam_data);

    return( (gboolean)value );
}

static gboolean
set_nwam_loc_boolean_prop( nwam_loc_handle_t loc, const char* prop_name, gboolean bool_value )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( loc == NULL ) {
        return( retval );
    }
    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_BOOLEAN ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_boolean ((boolean_t)bool_value, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a boolean value" );
        return retval;
    }

    if ( (nerr = nwam_loc_set_prop_value (loc, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);

    return( retval );
}

static guint64
get_nwam_loc_uint64_prop( nwam_loc_handle_t loc, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    uint64_t            value = 0;

    g_return_val_if_fail( prop_name != NULL, value );

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return value;
    }

    if ( (nerr = nwam_loc_get_prop_value (loc, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    if ( (nerr = nwam_value_get_uint64(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get uint64 value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    nwam_value_free(nwam_data);

    return( (guint64)value );
}

static gboolean
set_nwam_loc_uint64_prop( nwam_loc_handle_t loc, const char* prop_name, guint64 value )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( loc == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_uint64 (value, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a uint64 value" );
        return retval;
    }

    if ( (nerr = nwam_loc_set_prop_value (loc, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static guint64* 
get_nwam_loc_uint64_array_prop( nwam_loc_handle_t loc, const char* prop_name , guint *out_num )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    uint64_t           *value = NULL;
    uint_t              num = 0;
    guint64            *retval = NULL;

    g_return_val_if_fail( prop_name != NULL && out_num != NULL, retval );

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return value;
    }

    if ( (nerr = nwam_loc_get_prop_value (loc, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    if ( (nerr = nwam_value_get_uint64_array(nwam_data, &value, &num )) != NWAM_SUCCESS ) {
        g_debug("Unable to get uint64 value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    retval = (guint64*)g_malloc( sizeof(guint64) * num );
    for ( int i = 0; i < num; i++ ) {
        retval[i] = (guint64)value[i];
    }

    nwam_value_free(nwam_data);

    *out_num = num;

    return( retval );
}

static gboolean
set_nwam_loc_uint64_array_prop( nwam_loc_handle_t loc, const char* prop_name, 
                                const guint64 *value_array, guint len )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( loc == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_loc_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for loc property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_uint64_array ((uint64_t*)value_array, len, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a uint64 array value" );
        return retval;
    }

    if ( (nerr = nwam_loc_set_prop_value (loc, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for loc property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}


static GList*
convert_name_services_uint64_array_to_glist( guint64* ns_list, guint count )
{
    GList*  new_list = NULL;

    if ( ns_list != NULL ) {
        for ( int i = 0; i < count; i++ ) {
            /* Right now there is a 1-1 mapping between enums, so no real
             * conversion needed, just store the value */
            new_list = g_list_append( new_list, (gpointer)ns_list[i] );
        }
    }

    return( new_list );
}

static guint64*
convert_name_services_glist_to_unint64_array( GList* ns_glist, guint *count )
{
    guint64*    ns_list = NULL;

    if ( ns_glist != NULL && count != NULL ) {
        int     list_len = g_list_length( ns_glist );
        int     i = 0;

        ns_list = (guint64*)g_malloc0( sizeof(guint64) * (list_len) );

        i = 0;
        for ( GList *element  = g_list_first( ns_glist );
              element != NULL;
              element = g_list_next( element ) ) {
            guint64 ns = (guint64) element->data;
            /* Make sure it's a valid value */
            if ( ns >= NWAM_NAMESERVICES_DNS && ns <= NWAM_NAMESERVICES_LDAP ) {
                ns_list[i]  = (guint64)ns;
                i++;
            }
            else {
                g_error("Got unexpected nameservice value %ul", ns );
            }
        }
        *count = list_len;
    }

    return( ns_list );
}

#if 0
static void 
populate_env_with_handle( NwamuiEnv* env, nwam_loc_handle_t prv->nwam_loc )
{
    NwamuiEnvPrivate   *prv = env->prv;
    gchar             **condition_str = NULL;
    guint               num_nameservices = 0;
    nwam_nameservices_t *nameservices = NULL;

    prv->modifiable = !get_nwam_loc_boolean_prop( prv->nwam_loc, NWAM_LOC_PROP_READ_ONLY );
    prv->activation_mode = (nwamui_cond_activation_mode_t)get_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_ACTIVATION_MODE );
    condition_str = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_CONDITIONS );
    prv->conditions = nwamui_util_map_condition_strings_to_object_list( condition_str);
    g_strfreev( condition_str );

    prv->enabled = get_nwam_loc_boolean_prop( prv->nwam_loc, NWAM_LOC_PROP_ENABLED );

    /* Nameservice location properties */
    nameservices = (nwam_nameservices_t*)get_nwam_loc_uint64_array_prop(
                                           prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES, &num_nameservices );
    prv->nameservices = convert_name_services_array_to_glist( nameservices, num_nameservices );
    prv->nameservices_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES_CONFIG_FILE );
    prv->default_domainname = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_DEFAULT_DOMAIN );
    prv->dns_nameservice_domain = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_DOMAIN );
    prv->dns_nameservice_servers = nwamui_util_strv_to_glist(
            get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SERVERS ) );
    prv->dns_nameservice_search = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SEARCH );
    prv->nis_nameservice_servers = nwamui_util_strv_to_glist(
        get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_NIS_NAMESERVICE_SERVERS ) );
    prv->ldap_nameservice_servers = nwamui_util_strv_to_glist(
        get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_LDAP_NAMESERVICE_SERVERS ) );

    /* Path to hosts/ipnodes database */
    prv->hosts_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_HOSTS_FILE );

    /* NFSv4 domain */
    prv->nfsv4_domain = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NFSV4_DOMAIN );

    /* IPfilter configuration */
    prv->ipfilter_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_CONFIG_FILE );
    prv->ipfilter_v6_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_V6_CONFIG_FILE );
    prv->ipnat_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPNAT_CONFIG_FILE );
    prv->ippool_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPPOOL_CONFIG_FILE );

    /* IPsec configuration */
    prv->ike_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IKE_CONFIG_FILE );
    prv->ipsecpolicy_config_file = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPSECPOLICY_CONFIG_FILE );

    /* List of SMF services to enable/disable */
    prv->svcs_enable = nwamui_util_strv_to_glist(
            get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_SVCS_ENABLE ) );
    prv->svcs_disable = nwamui_util_strv_to_glist(
        get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_SVCS_DISABLE ) );
}
#endif /* 0 */

/**
 * nwamui_object_real_can_rename:
 * @object: a #NwamuiEnv.
 * @returns: TRUE if the name.can be changed.
 *
 **/
static gboolean
nwamui_object_real_can_rename (NwamuiObject *object)
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    NwamuiEnvPrivate *prv = NWAMUI_ENV(object)->prv;

    g_return_val_if_fail (NWAMUI_IS_ENV (object), FALSE);

    if ( prv->nwam_loc != NULL ) {
        if (nwam_loc_can_set_name( prv->nwam_loc )) {
            return( TRUE );
        }
    }
    return FALSE;
}

/** 
 * nwamui_env_set_name:
 * @nwamui_env: a #NwamuiEnv.
 * @name: The name to set.
 * 
 **/ 
static gboolean
nwamui_object_real_set_name (NwamuiObject *object, const gchar*  name )
{
    NwamuiEnvPrivate *prv  = NWAMUI_ENV_GET_PRIVATE(object);
    nwam_error_t      nerr;

    g_return_val_if_fail(NWAMUI_IS_ENV(object), FALSE);
    g_return_val_if_fail(name, FALSE);

    /* Initially set name or rename. */
    if (prv->name) {
        /* we may rename here */

        if (prv->nwam_loc != NULL) {
            nerr = nwam_loc_set_name (prv->nwam_loc, name);
            if (nerr != NWAM_SUCCESS) {
                g_debug ("nwam_loc_set_name %s error: %s", name, nwam_strerror (nerr));
                return FALSE;
            }
        } else {
            g_warning("Unexpected null loc handle");
        }
        prv->nwam_loc_modified = TRUE;
        g_free(prv->name);
    }

    prv->name = g_strdup(name);
    return TRUE;
}

/**
 * nwamui_env_clone:
 * @returns: a copy of an existing #NwamuiEnv.
 *
 * Creates a new #NwamuiEnv and copies properties.
 *
 **/
static NwamuiObject*
nwamui_object_real_clone(NwamuiObject *object, const gchar *name, NwamuiObject *parent)
{
    NwamuiEnv         *self   = NWAMUI_ENV(object);
    NwamuiObject      *new_env;
    nwam_error_t       nerr;
    nwam_loc_handle_t  new_env_h;
    NwamuiEnvPrivate *new_prv;

        
    g_assert(NWAMUI_IS_ENV(object));
    g_return_val_if_fail(name != NULL, NULL);

    nerr = nwam_loc_copy (self->prv->nwam_loc, name, &new_env_h);

    if ( nerr != NWAM_SUCCESS ) { 
        return( NULL );
    }

    /* new_env = nwamui_env_new_with_handle(new_env_h); */
    new_env = NWAMUI_OBJECT(g_object_new(NWAMUI_TYPE_ENV,
        "name", name,
        NULL));
    new_prv = NWAMUI_ENV_GET_PRIVATE(new_env);
    new_prv->nwam_loc = new_env_h;
    new_prv->nwam_loc_modified = TRUE;

    return new_env;
}

static gint
nwamui_object_real_open(NwamuiObject *object, const gchar *name, gint flag)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(object);
    nwam_error_t      nerr;

    g_assert(name);

    if (flag == NWAMUI_OBJECT_CREATE) {
        nerr = nwam_loc_create(name, &prv->nwam_loc);

        if (nerr == NWAM_SUCCESS) {
            prv->nwam_loc_modified = TRUE;
        } else {
            g_warning("nwamui_loc_create error creating nwam_loc_handle %s", name);
            prv->nwam_loc = NULL;
        }
    } else if (flag == NWAMUI_OBJECT_OPEN) {
        nwam_loc_handle_t  handle;

        nerr = nwam_loc_read(name, 0, &handle);
        if (nerr == NWAM_SUCCESS) {
            if (prv->nwam_loc) {
                nwam_loc_free(prv->nwam_loc);
            }
            prv->nwam_loc = handle;
        } else if (nerr == NWAM_ENTITY_NOT_FOUND) {
            /* Most likely only exists in memory right now, so we should use
             * handle passed in as parameter. In clone mode, the new handle
             * gets from nwam_env_copy can't be read again.
             */
            g_debug("Failed to read loc information for %s error: %s", name, nwam_strerror(nerr));
        } else {
            g_warning("Failed to read loc information for %s error: %s", name, nwam_strerror(nerr));
            prv->nwam_loc = NULL;
        }
    } else {
        g_assert_not_reached();
    }
    return nerr;
}

/**
 * nwamui_env_reload:   re-load stored configuration
 **/
static void
nwamui_object_real_reload(NwamuiObject* object)
{
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(object);
    gboolean           enabled = FALSE;

    g_return_if_fail(NWAMUI_IS_ENV(object));

    nwamui_object_real_open(object, prv->name, NWAMUI_OBJECT_OPEN);

    /* nwamui_object_set_handle will cause re-read from configuration */
    g_object_freeze_notify(G_OBJECT(object));

    /* Tell GUI to refresh */
    g_object_notify(G_OBJECT(object), "activation-mode");

    /* Initialise enabled to be the original value */
    enabled = get_nwam_loc_boolean_prop( prv->nwam_loc, NWAM_LOC_PROP_ENABLED );

    if ( prv->enabled != enabled ) {
        g_object_notify(G_OBJECT(object), "enabled" );
    }
    prv->enabled = enabled;

    prv->nwam_loc_modified = FALSE;

    g_object_thaw_notify(G_OBJECT(object));
}

/**
 * nwamui_env_get_name:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the name.
 *
 **/
static const gchar*
nwamui_object_real_get_name (NwamuiObject *object)
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    nwam_error_t    nerr;

    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);

    if (self->prv->name == NULL) {
        char *name;
        nerr = nwam_loc_get_name (self->prv->nwam_loc, &name);
        if (nerr != NWAM_SUCCESS) {
            g_debug ("nwam_loc_get_name %s error: %s", self->prv->name, nwam_strerror (nerr));
        }
        if (g_ascii_strcasecmp (self->prv->name, name) != 0) {
            g_assert_not_reached ();
        }
        free (name);
    }
    return( self->prv->name );
}

/** 
 * nwamui_env_set_enabled:
 * @nwamui_env: a #NwamuiEnv.
 * @enabled: Value to set enabled to.
 * 
 **/ 
static void
nwamui_object_real_set_enabled(NwamuiObject *object, gboolean enabled )
{
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(object);
    g_return_if_fail(NWAMUI_IS_ENV(object));

    if (prv->enabled != enabled) {
        prv->enabled = enabled;
        g_object_notify(G_OBJECT(object), "enabled" );
        prv->nwam_loc_modified = TRUE;
    }
}

/**
 * nwamui_env_get_enabled:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the enabled.
 *
 **/
static gboolean
nwamui_object_real_get_enabled(NwamuiObject *object)
{
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(object);
    g_return_val_if_fail(NWAMUI_IS_ENV(object), FALSE);

    return prv->enabled;
}

/**
 * nwamui_env_get_active:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: whether it is the active location.
 *
 **/
static gboolean
nwamui_object_real_get_active (NwamuiObject *object)
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    gboolean  active = FALSE; 
    g_return_val_if_fail (NWAMUI_IS_ENV (self), active);

    if ( self->prv->nwam_loc ) {
        nwam_state_t    state = NWAM_STATE_OFFLINE;
        nwam_aux_state_t    aux_state = NWAM_AUX_STATE_UNINITIALIZED;

        /* Use cached state in nwamui_object... */
        state = nwamui_object_get_nwam_state( NWAMUI_OBJECT(self), &aux_state, NULL);
        if ( state == NWAM_STATE_ONLINE && aux_state == NWAM_AUX_STATE_ACTIVE ) {
            active = TRUE;
        }
    }

    return( active );
}

/** 
 * nwamui_env_set_active
 * @nwamui_env: a #NwamuiEnv.
 * @active: Immediately activates/deactivates the env.
 * 
 * Sets activation of Envs to be manual with immediate effect.
 *
 * This entails doing the following:
 *
 * - If Manual
 *
 *   Specifcially activate the passed environment.
 *
 * - If not Manual
 *
 *   Loop through each Env and ensure it's disabled, returning control to the
 *   system to decide on best selection.
 *
 **/ 
static void
nwamui_object_real_set_active (NwamuiObject *object, gboolean active )
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    g_return_if_fail (NWAMUI_IS_ENV (self));

    /* Activate immediately */
    if ( active ) {
        nwam_error_t nerr;
        if ( (nerr = nwam_loc_enable (self->prv->nwam_loc)) != NWAM_SUCCESS ) {
            g_warning("Failed to enable location due to error: %s", nwam_strerror(nerr));
        }
    }
    else {
        nwam_error_t nerr;
        if ( (nerr = nwam_loc_disable (self->prv->nwam_loc)) != NWAM_SUCCESS ) {
            g_warning("Failed to disable location due to error: %s", nwam_strerror(nerr));
        }
    }
}


/** 
 * nwamui_env_set_nameservices:
 * @nwamui_env: a #NwamuiEnv.
 * @nameservices: should be a GList of nwamui_env_nameservices_t nameservices.
 * 
 **/ 
extern void
nwamui_env_set_nameservices(NwamuiEnv *self, const GList* nameservices)
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate *prv      = NWAMUI_ENV_GET_PRIVATE(self);
    guint64          *ns_array = NULL;
    guint             count    = 0;

    ns_array = convert_name_services_glist_to_unint64_array((GList*)nameservices, &count);
    set_nwam_loc_uint64_array_prop(prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES, ns_array, count);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "nameservices");
}

/**
 * nwamui_env_get_nameservices:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: a GList of nwamui_env_nameservices_t nameservices.
 *
 **/
extern GList*  
nwamui_env_get_nameservices (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate *prv              = NWAMUI_ENV_GET_PRIVATE(self);
    guint             num_nameservices = 0;
    guint64 *ns_64                     = (guint64*)get_nwam_loc_uint64_array_prop(
        prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES, &num_nameservices );
    GList            *ns_list          = convert_name_services_uint64_array_to_glist( ns_64, num_nameservices );
    g_free(ns_64);
    return ns_list;
}

/** 
 * nwamui_env_set_nameservices_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @nameservices_config_file: Value to set nameservices_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_nameservices_config_file (   NwamuiEnv *self,
                              const gchar*  nameservices_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES_CONFIG_FILE, 
      nameservices_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "nameservices_config_file");
}

/**
 * nwamui_env_get_nameservices_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the nameservices_config_file.
 *
 **/
extern gchar*
nwamui_env_get_nameservices_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NAMESERVICES_CONFIG_FILE );

    return str;
}

/** 
 * nwamui_env_set_default_domainname:
 * @nwamui_env: a #NwamuiEnv.
 * @default_domainname: Value to set default_domainname to.
 * 
 **/ 
extern void
nwamui_env_set_default_domainname (   NwamuiEnv *self,
                              const gchar*  default_domainname )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_DEFAULT_DOMAIN, 
      default_domainname);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "default_domainname");
}

/**
 * nwamui_env_get_default_domainname:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the default_domainname.
 *
 **/
extern gchar*
nwamui_env_get_default_domainname (NwamuiEnv *self)
{
    gchar*  default_domainname = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), default_domainname);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    default_domainname = get_nwam_loc_string_prop(prv->nwam_loc, NWAM_LOC_PROP_DEFAULT_DOMAIN);

    return( default_domainname );
}

/** 
 * nwamui_env_set_dns_nameservice_domain:
 * @nwamui_env: a #NwamuiEnv.
 * @dns_nameservice_domain: Value to set dns_nameservice_domain to.
 * 
 **/ 
extern void
nwamui_env_set_dns_nameservice_domain (   NwamuiEnv *self,
                              const gchar*  dns_nameservice_domain)
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_DOMAIN,
      dns_nameservice_domain);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_domain");
}

/**
 * nwamui_env_get_dns_nameservice_domain:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the dns_nameservice_domain.
 *
 **/
extern gchar*
nwamui_env_get_dns_nameservice_domain (NwamuiEnv *self)
{
    gchar*  dns_nameservice_domain = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), dns_nameservice_domain);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    dns_nameservice_domain = get_nwam_loc_string_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_DOMAIN);

    return( dns_nameservice_domain );
}

/** 
 * nwamui_env_set_dns_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @dns_nameservice_config_source: Value to set dns_nameservice_config_source to.
 * 
 **/ 
extern void
nwamui_env_set_dns_nameservice_config_source (   NwamuiEnv *self,
                                                 nwamui_env_config_source_t        dns_nameservice_config_source )
{
    g_return_if_fail (NWAMUI_IS_ENV(self));
    g_assert (dns_nameservice_config_source >= NWAMUI_ENV_CONFIG_SOURCE_MANUAL 
              && dns_nameservice_config_source <= NWAMUI_COND_ACTIVATION_MODE_LAST );
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_CONFIGSRC, dns_nameservice_config_source);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_config_source");
}

/**
 * nwamui_env_get_dns_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the dns_nameservice_config_source.
 *
 **/
extern nwamui_env_config_source_t
nwamui_env_get_dns_nameservice_config_source (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NWAMUI_ENV_CONFIG_SOURCE_DHCP);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    return (gint)get_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_CONFIGSRC );
}

/** 
 * nwamui_env_set_dns_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @dns_nameservice_servers: Value to set dns_nameservice_servers to.
 * 
 **/ 
extern void
nwamui_env_set_dns_nameservice_servers (   NwamuiEnv *self,
                              const GList*    dns_nameservice_servers )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar** ns_server_strs = nwamui_util_glist_to_strv((GList*)dns_nameservice_servers);
    set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SERVERS, ns_server_strs, 0 );
    g_strfreev(ns_server_strs);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_servers");
}

/**
 * nwamui_env_get_dns_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the dns_nameservice_servers.
 *
 **/
extern GList*  
nwamui_env_get_dns_nameservice_servers (NwamuiEnv *self)
{
    GList*    dns_nameservice_servers = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), dns_nameservice_servers);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SERVERS );
    dns_nameservice_servers = nwamui_util_strv_to_glist( strv );
    g_strfreev( strv );

    return( dns_nameservice_servers );
}

/** 
 * nwamui_env_set_dns_nameservice_search:
 * @nwamui_env: a #NwamuiEnv.
 * @dns_nameservice_search: Value to set dns_nameservice_search to.
 * 
 **/ 
extern void
nwamui_env_set_dns_nameservice_search (   NwamuiEnv *self,
                                          const GList*  dns_nameservice_search )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar** ns_server_strs = nwamui_util_glist_to_strv((GList*)dns_nameservice_search);
    /* We may need to/from convert to , separated string?? */
    set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SEARCH, ns_server_strs, 0 );
    g_strfreev(ns_server_strs);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_search");
}

/**
 * nwamui_env_get_dns_nameservice_search:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the dns_nameservice_search list.
 *
 **/
extern GList*
nwamui_env_get_dns_nameservice_search (NwamuiEnv *self)
{
    GList*  dns_nameservice_search = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), dns_nameservice_search);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SEARCH );
    /* We may need to/from convert to , separated string?? */
    dns_nameservice_search = nwamui_util_strv_to_glist( strv );
    g_strfreev( strv );

    return( dns_nameservice_search );
}

extern void
nwamui_env_set_dns_nameservice_options(NwamuiEnv *self, const gchar* dns_nameservice_options)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(self);

    g_return_if_fail (NWAMUI_IS_ENV (self));

    set_nwam_loc_string_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_OPTIONS, dns_nameservice_options);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_options");

}

extern gchar*
nwamui_env_get_dns_nameservice_options(NwamuiEnv *self)
{
    NwamuiEnvPrivate *prv     = NWAMUI_ENV_GET_PRIVATE(self);
    gchar            *dns_nameservice_options = NULL; 

    g_return_val_if_fail(NWAMUI_IS_ENV (self), dns_nameservice_options);

    dns_nameservice_options = get_nwam_loc_string_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_OPTIONS);

    return dns_nameservice_options;
}

/** 
 * nwamui_env_set_dns_nameservice_sortlist:
 * @nwamui_env: a #NwamuiEnv.
 * @dns_nameservice_sortlist: Value to set dns_nameservice_sortlist to.
 * 
 **/ 
extern void
nwamui_env_set_dns_nameservice_sortlist(NwamuiEnv *self, const GList*  dns_nameservice_sortlist )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);
    gint len = g_list_length((GList*)dns_nameservice_sortlist);
    gchar** ns_server_strs = NULL;

    if (len > 0) {
        ns_server_strs = g_malloc0(len * sizeof(gchar*));
        ns_server_strs[len] = NULL;

        for (gint i = 0;
             dns_nameservice_sortlist;
             dns_nameservice_sortlist = g_list_next(dns_nameservice_sortlist), i++) {
            gchar *addr;
            gchar *subnet;
            NwamuiIp *ip = NWAMUI_IP(dns_nameservice_sortlist->data);
            addr = nwamui_ip_get_address(ip);
            subnet = nwamui_ip_get_subnet_prefix(ip);
            ns_server_strs[i] = g_strdup_printf("%s/%s", addr, subnet);
            g_free(addr);
            g_free(subnet);
        }

        set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SORTLIST, ns_server_strs, 0);
        g_strfreev(ns_server_strs);
    } else {
        set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SORTLIST, NULL, 0);
    }
    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "dns_nameservice_sortlist");
}

/**
 * nwamui_env_get_dns_nameservice_sortlist:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the dns_nameservice_sortlist list.
 *
 **/
extern GList*
nwamui_env_get_dns_nameservice_sortlist(NwamuiEnv *self)
{
    GList*  dns_nameservice_sortlist = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), dns_nameservice_sortlist);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar **strv = get_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_DNS_NAMESERVICE_SORTLIST);
    for (gchar **str = strv; str && *str; str++) {
        gchar **inf = g_strsplit(*str, "/", 2);

        dns_nameservice_sortlist = g_list_append(dns_nameservice_sortlist,
          nwamui_ip_new(NULL, (inf[0]?inf[0]:""), (inf[1]?inf[1]:""), FALSE, FALSE, FALSE));

        g_strfreev(inf);
    }
    g_strfreev(strv);

    return dns_nameservice_sortlist;
}

/** 
 * nwamui_env_set_nis_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @nis_nameservice_config_source: Value to set nis_nameservice_config_source to.
 * 
 **/ 
extern void
nwamui_env_set_nis_nameservice_config_source (   NwamuiEnv *self,
                                                 nwamui_env_config_source_t        nis_nameservice_config_source )
{
    g_return_if_fail (NWAMUI_IS_ENV(self));
    g_assert (nis_nameservice_config_source >= NWAMUI_ENV_CONFIG_SOURCE_MANUAL 
              && nis_nameservice_config_source <= NWAMUI_COND_ACTIVATION_MODE_LAST );
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_NIS_NAMESERVICE_CONFIGSRC, nis_nameservice_config_source);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "nis_nameservice_config_source");
}

/**
 * nwamui_env_get_nis_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the nis_nameservice_config_source.
 *
 **/
extern nwamui_env_config_source_t
nwamui_env_get_nis_nameservice_config_source (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NWAMUI_ENV_CONFIG_SOURCE_DHCP);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    return (gint)get_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_NIS_NAMESERVICE_CONFIGSRC );
}

/** 
 * nwamui_env_set_nis_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @nis_nameservice_servers: Value to set nis_nameservice_servers to.
 * 
 **/ 
extern void
nwamui_env_set_nis_nameservice_servers (   NwamuiEnv *self,
                              const GList*    nis_nameservice_servers )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar** ns_server_strs = nwamui_util_glist_to_strv((GList*)nis_nameservice_servers);
    set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_NIS_NAMESERVICE_SERVERS, ns_server_strs, 0 );
    g_strfreev(ns_server_strs);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "nis_nameservice_servers");
}

/**
 * nwamui_env_get_nis_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the nis_nameservice_servers.
 *
 **/
extern GList*  
nwamui_env_get_nis_nameservice_servers (NwamuiEnv *self)
{
    GList*    nis_nameservice_servers = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), nis_nameservice_servers);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_NIS_NAMESERVICE_SERVERS );
    nis_nameservice_servers = nwamui_util_strv_to_glist( strv );
    g_strfreev( strv );

    return( nis_nameservice_servers );
}

/** 
 * nwamui_env_set_ldap_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @ldap_nameservice_config_source: Value to set ldap_nameservice_config_source to.
 * 
 **/ 
extern void
nwamui_env_set_ldap_nameservice_config_source (   NwamuiEnv *self,
                                                 nwamui_env_config_source_t        ldap_nameservice_config_source )
{
    g_return_if_fail (NWAMUI_IS_ENV(self));
    g_assert (ldap_nameservice_config_source >= NWAMUI_ENV_CONFIG_SOURCE_MANUAL 
              && ldap_nameservice_config_source <= NWAMUI_COND_ACTIVATION_MODE_LAST );
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_uint64_prop(prv->nwam_loc, NWAM_LOC_PROP_LDAP_NAMESERVICE_CONFIGSRC, ldap_nameservice_config_source);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ldap_nameservice_config_source");
}

/**
 * nwamui_env_get_ldap_nameservice_config_source:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ldap_nameservice_config_source.
 *
 **/
extern nwamui_env_config_source_t
nwamui_env_get_ldap_nameservice_config_source (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NWAMUI_ENV_CONFIG_SOURCE_DHCP);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    return (gint)get_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_LDAP_NAMESERVICE_CONFIGSRC );
}

/** 
 * nwamui_env_set_ldap_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @ldap_nameservice_servers: Value to set ldap_nameservice_servers to.
 * 
 **/ 
extern void
nwamui_env_set_ldap_nameservice_servers (   NwamuiEnv *self,
                              const GList*    ldap_nameservice_servers )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar** ns_server_strs = nwamui_util_glist_to_strv((GList*)ldap_nameservice_servers);
    set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_LDAP_NAMESERVICE_SERVERS, ns_server_strs, 0 );
    g_strfreev(ns_server_strs);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ldap_nameservice_servers");
}

/**
 * nwamui_env_get_ldap_nameservice_servers:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ldap_nameservice_servers.
 *
 **/
extern GList*  
nwamui_env_get_ldap_nameservice_servers (NwamuiEnv *self)
{
    GList*    ldap_nameservice_servers = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), ldap_nameservice_servers);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar **strv = get_nwam_loc_string_array_prop( prv->nwam_loc, NWAM_LOC_PROP_LDAP_NAMESERVICE_SERVERS );
    ldap_nameservice_servers = nwamui_util_strv_to_glist( strv );
g_strfreev( strv );

    return( ldap_nameservice_servers );
}

#ifndef _DISABLE_HOSTS_FILE
/** 
 * nwamui_env_set_hosts_file:
 * @nwamui_env: a #NwamuiEnv.
 * @hosts_file: Value to set hosts_file to.
 * 
 **/ 
extern void
nwamui_env_set_hosts_file (   NwamuiEnv *self,
                              const gchar*  hosts_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_HOSTS_FILE, 
      hosts_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "hosts_file");
}

/**
 * nwamui_env_get_hosts_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the hosts_file.
 *
 **/
extern gchar*
nwamui_env_get_hosts_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), hosts_file);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_HOSTS_FILE );

    return str;
}
#endif /* _DISABLE_HOSTS_FILE */

/** 
 * nwamui_env_set_nfsv4_domain:
 * @nwamui_env: a #NwamuiEnv.
 * @nfsv4_domain: Value to set nfsv4_domain to.
 * 
 **/ 
extern void
nwamui_env_set_nfsv4_domain (   NwamuiEnv *self,
                              const gchar*  nfsv4_domain )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NFSV4_DOMAIN, 
      nfsv4_domain);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "nfsv4_domain");
}

/**
 * nwamui_env_get_nfsv4_domain:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the nfsv4_domain.
 *
 **/
extern gchar*
nwamui_env_get_nfsv4_domain (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_NFSV4_DOMAIN );
    return str;
}

/** 
 * nwamui_env_set_ipfilter_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ipfilter_config_file: Value to set ipfilter_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ipfilter_config_file (   NwamuiEnv *self,
                              const gchar*  ipfilter_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_CONFIG_FILE, 
      ipfilter_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ipfilter_config_file");
}

/**
 * nwamui_env_get_ipfilter_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ipfilter_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ipfilter_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_CONFIG_FILE );
    return str;
}

/** 
 * nwamui_env_set_ipfilter_v6_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ipfilter_v6_config_file: Value to set ipfilter_v6_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ipfilter_v6_config_file (   NwamuiEnv *self,
                              const gchar*  ipfilter_v6_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_V6_CONFIG_FILE, 
      ipfilter_v6_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ipfilter_v6_config_file");
}

/**
 * nwamui_env_get_ipfilter_v6_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ipfilter_v6_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ipfilter_v6_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPFILTER_V6_CONFIG_FILE );
    return str;
}

/** 
 * nwamui_env_set_ipnat_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ipnat_config_file: Value to set ipnat_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ipnat_config_file (   NwamuiEnv *self,
                              const gchar*  ipnat_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPNAT_CONFIG_FILE, 
      ipnat_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ipnat_config_file");
}

/**
 * nwamui_env_get_ipnat_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ipnat_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ipnat_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPNAT_CONFIG_FILE );
    return str;
}

/** 
 * nwamui_env_set_ippool_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ippool_config_file: Value to set ippool_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ippool_config_file (   NwamuiEnv *self,
                              const gchar*  ippool_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPPOOL_CONFIG_FILE, 
      ippool_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ippool_config_file");
}

/**
 * nwamui_env_get_ippool_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ippool_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ippool_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPPOOL_CONFIG_FILE );
    return str;
}

/** 
 * nwamui_env_set_ike_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ike_config_file: Value to set ike_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ike_config_file (   NwamuiEnv *self,
                              const gchar*  ike_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IKE_CONFIG_FILE, 
      ike_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ike_config_file");
}

/**
 * nwamui_env_get_ike_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ike_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ike_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IKE_CONFIG_FILE );
    return str;
}

/** 
 * nwamui_env_set_ipsecpolicy_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @ipsecpolicy_config_file: Value to set ipsecpolicy_config_file to.
 * 
 **/ 
extern void
nwamui_env_set_ipsecpolicy_config_file (   NwamuiEnv *self,
                              const gchar*  ipsecpolicy_config_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    set_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPSECPOLICY_CONFIG_FILE, 
      ipsecpolicy_config_file);

    prv->nwam_loc_modified = TRUE;
	g_object_notify(G_OBJECT(self), "ipsecpolicy_config_file");
}

/**
 * nwamui_env_get_ipsecpolicy_config_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the ipsecpolicy_config_file.
 *
 **/
extern gchar*
nwamui_env_get_ipsecpolicy_config_file (NwamuiEnv *self)
{
    g_return_val_if_fail (NWAMUI_IS_ENV (self), NULL);
    NwamuiEnvPrivate  *prv     = NWAMUI_ENV_GET_PRIVATE(self);

    gchar* str = get_nwam_loc_string_prop( prv->nwam_loc, NWAM_LOC_PROP_IPSECPOLICY_CONFIG_FILE );
    return str;
}

#ifdef ENABLE_NETSERVICES
/** 
 * nwamui_env_set_svcs_enable:
 * @nwamui_env: a #NwamuiEnv.
 * @svcs_enable: Value to set svcs_enable to. NULL means remove all.
 * 
 **/ 
extern void
nwamui_env_set_svcs_enable (   NwamuiEnv *self,
                              const GList*    svcs_enable )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));

    g_object_set (G_OBJECT (self),
      "svcs_enable", svcs_enable,
      NULL);
}

/**
 * nwamui_env_get_svcs_enable:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the svcs_enable.
 *
 **/
extern GList*  
nwamui_env_get_svcs_enable (NwamuiEnv *self)
{
    GList*    svcs_enable = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), svcs_enable);

    g_object_get (G_OBJECT (self),
                  "svcs_enable", &svcs_enable,
                  NULL);

    return( svcs_enable );
}

/** 
 * nwamui_env_set_svcs_disable:
 * @nwamui_env: a #NwamuiEnv.
 * @svcs_disable: Value to set svcs_disable to. NULL means remove all.
 * 
 **/ 
extern void
nwamui_env_set_svcs_disable (   NwamuiEnv *self,
                              const GList*    svcs_disable )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));

    g_object_set (G_OBJECT (self),
      "svcs_disable", svcs_disable,
      NULL);
}

/**
 * nwamui_env_get_svcs_disable:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the svcs_disable.
 *
 **/
extern GList*  
nwamui_env_get_svcs_disable (NwamuiEnv *self)
{
    GList*    svcs_disable = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), svcs_disable);

    g_object_get (G_OBJECT (self),
                  "svcs_disable", &svcs_disable,
                  NULL);

    return( svcs_disable );
}
#endif /* ENABLE_NETSERVICES */

#ifdef ENABLE_PROXY
/** 
 * nwamui_env_set_proxy_type:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_type: The proxy_type to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_type (   NwamuiEnv                *self,
                              nwamui_env_proxy_type_t   proxy_type )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_type >= NWAMUI_ENV_PROXY_TYPE_DIRECT && proxy_type <= NWAMUI_ENV_PROXY_TYPE_LAST-1 );

    g_object_set (G_OBJECT (self),
                  "proxy_type", proxy_type,
                  NULL);
}

/**
 * nwamui_env_get_proxy_type:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_type.
 *
 **/
extern nwamui_env_proxy_type_t
nwamui_env_get_proxy_type (NwamuiEnv *self)
{
    gint  proxy_type = NWAMUI_ENV_PROXY_TYPE_DIRECT; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_type);

    g_object_get (G_OBJECT (self),
                  "proxy_type", &proxy_type,
                  NULL);

    return( (nwamui_env_proxy_type_t)proxy_type );
}

/** 
 * nwamui_env_set_proxy_pac_file:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_pac_file: The proxy_pac_file to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_pac_file (   NwamuiEnv *self,
                              const gchar*  proxy_pac_file )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_pac_file != NULL );

    if ( proxy_pac_file != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_pac_file", proxy_pac_file,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_pac_file:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_pac_file.
 *
 **/
extern gchar*
nwamui_env_get_proxy_pac_file (NwamuiEnv *self)
{
    gchar*  proxy_pac_file = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_pac_file);

    g_object_get (G_OBJECT (self),
                  "proxy_pac_file", &proxy_pac_file,
                  NULL);

    return( proxy_pac_file );
}

/** 
 * nwamui_env_set_proxy_http_server:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_http_server: The proxy_http_server to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_http_server (   NwamuiEnv *self,
                              const gchar*  proxy_http_server )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_http_server != NULL );

    if ( proxy_http_server != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_http_server", proxy_http_server,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_http_server:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_http_server.
 *
 **/
extern gchar*
nwamui_env_get_proxy_http_server (NwamuiEnv *self)
{
    gchar*  proxy_http_server = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_http_server);

    g_object_get (G_OBJECT (self),
                  "proxy_http_server", &proxy_http_server,
                  NULL);

    return( proxy_http_server );
}

/** 
 * nwamui_env_set_proxy_https_server:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_https_server: The proxy_https_server to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_https_server (   NwamuiEnv *self,
                              const gchar*  proxy_https_server )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_https_server != NULL );

    if ( proxy_https_server != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_https_server", proxy_https_server,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_https_server:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_https_server.
 *
 **/
extern gchar*
nwamui_env_get_proxy_https_server (NwamuiEnv *self)
{
    gchar*  proxy_https_server = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_https_server);

    g_object_get (G_OBJECT (self),
                  "proxy_https_server", &proxy_https_server,
                  NULL);

    return( proxy_https_server );
}

/** 
 * nwamui_env_set_proxy_ftp_server:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_ftp_server: The proxy_ftp_server to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_ftp_server (   NwamuiEnv *self,
                              const gchar*  proxy_ftp_server )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_ftp_server != NULL );

    if ( proxy_ftp_server != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_ftp_server", proxy_ftp_server,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_ftp_server:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_ftp_server.
 *
 **/
extern gchar*
nwamui_env_get_proxy_ftp_server (NwamuiEnv *self)
{
    gchar*  proxy_ftp_server = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_ftp_server);

    g_object_get (G_OBJECT (self),
                  "proxy_ftp_server", &proxy_ftp_server,
                  NULL);

    return( proxy_ftp_server );
}

/** 
 * nwamui_env_set_proxy_gopher_server:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_gopher_server: The proxy_gopher_server to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_gopher_server (   NwamuiEnv *self,
                              const gchar*  proxy_gopher_server )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_gopher_server != NULL );

    if ( proxy_gopher_server != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_gopher_server", proxy_gopher_server,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_gopher_server:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_gopher_server.
 *
 **/
extern gchar*
nwamui_env_get_proxy_gopher_server (NwamuiEnv *self)
{
    gchar*  proxy_gopher_server = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_gopher_server);

    g_object_get (G_OBJECT (self),
                  "proxy_gopher_server", &proxy_gopher_server,
                  NULL);

    return( proxy_gopher_server );
}

/** 
 * nwamui_env_set_proxy_socks_server:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_socks_server: The proxy_socks_server to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_socks_server (   NwamuiEnv *self,
                              const gchar*  proxy_socks_server )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_socks_server != NULL );

    if ( proxy_socks_server != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_socks_server", proxy_socks_server,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_socks_server:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_socks_server.
 *
 **/
extern gchar*
nwamui_env_get_proxy_socks_server (NwamuiEnv *self)
{
    gchar*  proxy_socks_server = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_socks_server);

    g_object_get (G_OBJECT (self),
                  "proxy_socks_server", &proxy_socks_server,
                  NULL);

    return( proxy_socks_server );
}

/** 
 * nwamui_env_set_proxy_bypass_list:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_bypass_list: The proxy_bypass_list to set.
 * 
 **/ 
extern void
nwamui_env_set_proxy_bypass_list (   NwamuiEnv *self,
                              const gchar*  proxy_bypass_list )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_bypass_list != NULL );

    if ( proxy_bypass_list != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_bypass_list", proxy_bypass_list,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_bypass_list:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_bypass_list.
 *
 **/
extern gchar*
nwamui_env_get_proxy_bypass_list (NwamuiEnv *self)
{
    gchar*  proxy_bypass_list = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_bypass_list);

    g_object_get (G_OBJECT (self),
                  "proxy_bypass_list", &proxy_bypass_list,
                  NULL);

    return( proxy_bypass_list );
}

/** 
 * nwamui_env_set_use_http_proxy_for_all:
 * @nwamui_env: a #NwamuiEnv.
 * @use_http_proxy_for_all: Value to set use_http_proxy_for_all to.
 * 
 **/ 
extern void
nwamui_env_set_use_http_proxy_for_all (   NwamuiEnv *self,
                              gboolean        use_http_proxy_for_all )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));

    g_object_set (G_OBJECT (self),
                  "use_http_proxy_for_all", use_http_proxy_for_all,
                  NULL);
}

/**
 * nwamui_env_get_use_http_proxy_for_all:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the use_http_proxy_for_all.
 *
 **/
extern gboolean
nwamui_env_get_use_http_proxy_for_all (NwamuiEnv *self)
{
    gboolean  use_http_proxy_for_all = FALSE; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), use_http_proxy_for_all);

    g_object_get (G_OBJECT (self),
                  "use_http_proxy_for_all", &use_http_proxy_for_all,
                  NULL);

    return( use_http_proxy_for_all );
}

/** 
 * nwamui_env_set_proxy_http_port:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_http_port: Value to set proxy_http_port to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_http_port (   NwamuiEnv *self,
                              gint        proxy_http_port )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_http_port >= 0 && proxy_http_port <= G_MAXINT );

    g_object_set (G_OBJECT (self),
                  "proxy_http_port", proxy_http_port,
                  NULL);
}

/**
 * nwamui_env_get_proxy_http_port:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_http_port.
 *
 **/
extern gint
nwamui_env_get_proxy_http_port (NwamuiEnv *self)
{
    gint  proxy_http_port = 80; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_http_port);

    g_object_get (G_OBJECT (self),
                  "proxy_http_port", &proxy_http_port,
                  NULL);

    return( proxy_http_port );
}

/** 
 * nwamui_env_set_proxy_https_port:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_https_port: Value to set proxy_https_port to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_https_port (   NwamuiEnv *self,
                              gint        proxy_https_port )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_https_port >= 0 && proxy_https_port <= G_MAXINT );

    g_object_set (G_OBJECT (self),
                  "proxy_https_port", proxy_https_port,
                  NULL);
}

/**
 * nwamui_env_get_proxy_https_port:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_https_port.
 *
 **/
extern gint
nwamui_env_get_proxy_https_port (NwamuiEnv *self)
{
    gint  proxy_https_port = 80; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_https_port);

    g_object_get (G_OBJECT (self),
                  "proxy_https_port", &proxy_https_port,
                  NULL);

    return( proxy_https_port );
}

/** 
 * nwamui_env_set_proxy_ftp_port:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_ftp_port: Value to set proxy_ftp_port to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_ftp_port (   NwamuiEnv *self,
                              gint        proxy_ftp_port )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_ftp_port >= 0 && proxy_ftp_port <= G_MAXINT );

    g_object_set (G_OBJECT (self),
                  "proxy_ftp_port", proxy_ftp_port,
                  NULL);
}

/**
 * nwamui_env_get_proxy_ftp_port:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_ftp_port.
 *
 **/
extern gint
nwamui_env_get_proxy_ftp_port (NwamuiEnv *self)
{
    gint  proxy_ftp_port = 80; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_ftp_port);

    g_object_get (G_OBJECT (self),
                  "proxy_ftp_port", &proxy_ftp_port,
                  NULL);

    return( proxy_ftp_port );
}

/** 
 * nwamui_env_set_proxy_gopher_port:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_gopher_port: Value to set proxy_gopher_port to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_gopher_port (   NwamuiEnv *self,
                              gint        proxy_gopher_port )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_gopher_port >= 0 && proxy_gopher_port <= G_MAXINT );

    g_object_set (G_OBJECT (self),
                  "proxy_gopher_port", proxy_gopher_port,
                  NULL);
}

/**
 * nwamui_env_get_proxy_gopher_port:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_gopher_port.
 *
 **/
extern gint
nwamui_env_get_proxy_gopher_port (NwamuiEnv *self)
{
    gint  proxy_gopher_port = 80; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_gopher_port);

    g_object_get (G_OBJECT (self),
                  "proxy_gopher_port", &proxy_gopher_port,
                  NULL);

    return( proxy_gopher_port );
}

/** 
 * nwamui_env_set_proxy_socks_port:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_socks_port: Value to set proxy_socks_port to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_socks_port (   NwamuiEnv *self,
                              gint        proxy_socks_port )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_socks_port >= 0 && proxy_socks_port <= G_MAXINT );

    g_object_set (G_OBJECT (self),
                  "proxy_socks_port", proxy_socks_port,
                  NULL);
}

/** 
 * nwamui_env_set_proxy_username:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_username: Value to set proxy_username to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_username (   NwamuiEnv *self,
                              const gchar*  proxy_username )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_username != NULL );

    if ( proxy_username != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_username", proxy_username,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_username:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_username.
 *
 **/
extern gchar*
nwamui_env_get_proxy_username (NwamuiEnv *self)
{
    gchar*  proxy_username = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_username);

    g_object_get (G_OBJECT (self),
                  "proxy_username", &proxy_username,
                  NULL);

    return( proxy_username );
}

/** 
 * nwamui_env_set_proxy_password:
 * @nwamui_env: a #NwamuiEnv.
 * @proxy_password: Value to set proxy_password to.
 * 
 **/ 
extern void
nwamui_env_set_proxy_password (   NwamuiEnv *self,
                              const gchar*  proxy_password )
{
    g_return_if_fail (NWAMUI_IS_ENV (self));
    g_assert (proxy_password != NULL );

    if ( proxy_password != NULL ) {
        g_object_set (G_OBJECT (self),
                      "proxy_password", proxy_password,
                      NULL);
    }
}

/**
 * nwamui_env_get_proxy_password:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_password.
 *
 **/
extern gchar*
nwamui_env_get_proxy_password (NwamuiEnv *self)
{
    gchar*  proxy_password = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_password);

    g_object_get (G_OBJECT (self),
                  "proxy_password", &proxy_password,
                  NULL);

    return( proxy_password );
}

/**
 * nwamui_env_get_proxy_socks_port:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the proxy_socks_port.
 *
 **/
extern gint
nwamui_env_get_proxy_socks_port (NwamuiEnv *self)
{
    gint  proxy_socks_port = 1080; 

    g_return_val_if_fail (NWAMUI_IS_ENV (self), proxy_socks_port);

    g_object_get (G_OBJECT (self),
                  "proxy_socks_port", &proxy_socks_port,
                  NULL);

    return( proxy_socks_port );
}
#endif /* ENABLE_PROXY */

/** 
 * nwamui_env_set_activation_mode:
 * @nwamui_env: a #NwamuiEnv.
 * @activation_mode: Value to set activation_mode to.
 * 
 **/ 
static void
nwamui_object_real_set_activation_mode (   NwamuiObject *object, gint activation_mode )
{
    NwamuiEnvPrivate *prv  = NWAMUI_ENV_GET_PRIVATE(object);

    g_return_if_fail (NWAMUI_IS_ENV(object));
    g_assert (activation_mode >= NWAMUI_COND_ACTIVATION_MODE_MANUAL && activation_mode <= NWAMUI_COND_ACTIVATION_MODE_LAST );

    set_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_ACTIVATION_MODE, activation_mode);

    prv->nwam_loc_modified = TRUE;
}

/**
 * nwamui_env_get_activation_mode:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the activation_mode.
 *
 **/
static gint
nwamui_object_real_get_activation_mode (NwamuiObject *object)
{
    NwamuiEnvPrivate *prv             = NWAMUI_ENV_GET_PRIVATE(object);
    gint              activation_mode = NWAMUI_COND_ACTIVATION_MODE_MANUAL; 

    g_return_val_if_fail (NWAMUI_IS_ENV (object), activation_mode);

    activation_mode = (gint)get_nwam_loc_uint64_prop( prv->nwam_loc, NWAM_LOC_PROP_ACTIVATION_MODE );

    return( (nwamui_cond_activation_mode_t)activation_mode );
}

/** 
 * nwamui_env_set_conditions:
 * @nwamui_env: a #NwamuiEnv.
 * @conditions: Value to set conditions to.
 * 
 **/ 
static void
nwamui_object_real_set_conditions(NwamuiObject *object, const GList* conditions )
{
    NwamuiEnvPrivate  *prv            = NWAMUI_ENV_GET_PRIVATE(object);
    char             **condition_strs = NULL;
    guint              len            = 0;

    g_return_if_fail(NWAMUI_IS_ENV(object));

    if ( conditions != NULL ) {
        condition_strs = nwamui_util_map_object_list_to_condition_strings((GList*)conditions, &len);
        set_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_CONDITIONS, condition_strs, len);
        if (condition_strs) {
            free(condition_strs);
        }
    } else {
        nwamui_object_real_set_activation_mode(object, (gint)NWAMUI_COND_ACTIVATION_MODE_MANUAL);
    }
    prv->nwam_loc_modified = TRUE;
}

/**
 * nwamui_env_get_conditions:
 * @nwamui_env: a #NwamuiEnv.
 * @returns: the conditions.
 *
 **/
static GList*
nwamui_object_real_get_conditions(NwamuiObject *object)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(object);
    gchar**           condition_strs;
    GList            *conditions;

    g_return_val_if_fail(NWAMUI_IS_ENV(object), conditions);

    condition_strs = get_nwam_loc_string_array_prop(prv->nwam_loc, NWAM_LOC_PROP_CONDITIONS );
    conditions = nwamui_util_map_condition_strings_to_object_list(condition_strs);

    g_strfreev( condition_strs );

    return conditions;
}

#if 0
/* These are not needed right now since we don't support property templates,
 * but would like to keep around for when we do.
 */
extern GtkTreeModel *
nwamui_env_get_svcs (NwamuiEnv *self)
{
    GtkTreeModel *model;
    
    g_object_get (G_OBJECT (self),
      "svcs", &model,
      NULL);

    return model;
}

extern NwamuiSvc*
nwamui_env_get_svc (NwamuiEnv *self, GtkTreeIter *iter)
{
    NwamuiSvc *svcobj = NULL;

    g_return_val_if_fail (NWAMUI_IS_ENV (self), svcobj);

    gtk_tree_model_get (GTK_TREE_MODEL (self->prv->svcs_model), iter, SVC_OBJECT, &svcobj, -1);

    return svcobj;
}

extern NwamuiSvc*
nwamui_env_find_svc (NwamuiEnv *self, const gchar *svc)
{
    NwamuiSvc *svcobj = NULL;
    GtkTreeIter iter;

    g_return_val_if_fail (svc, NULL);
    g_return_val_if_fail (NWAMUI_IS_ENV (self), svcobj);

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(self->prv->svcs_model), &iter)) {
        do {
            gchar *name;
            
            gtk_tree_model_get (GTK_TREE_MODEL(self->prv->svcs_model), &iter, SVC_OBJECT, &svcobj, -1);
            name = nwamui_svc_get_name (svcobj);
            
            if (g_ascii_strcasecmp (name, svc) == 0) {
                g_free (name);
                return svcobj;
            }
            g_free (name);
            g_object_unref (svcobj);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(self->prv->svcs_model), &iter));
    }
    return NULL;
}

extern void
nwamui_env_svc_remove (NwamuiEnv *self, GtkTreeIter *iter)
{
    gtk_list_store_remove (GTK_LIST_STORE(self->prv->svcs_model), iter);
}

extern void
nwamui_env_svc_foreach (NwamuiEnv *self, GtkTreeModelForeachFunc func, gpointer data)
{
    gtk_tree_model_foreach (GTK_TREE_MODEL(self->prv->svcs_model), func, data);
}

extern gboolean
nwamui_env_svc_insert (NwamuiEnv *self, NwamuiSvc *svc)
{
	GtkTreeIter iter;
    NwamuiSvc *svcobj;
    nwam_error_t nerr;

    gtk_list_store_append (GTK_LIST_STORE(self->prv->svcs_model), &iter);
    gtk_list_store_set (GTK_LIST_STORE(self->prv->svcs_model), &iter,
      SVC_OBJECT, svcobj, -1);
    
    return TRUE;
}

extern gboolean
nwamui_env_svc_delete (NwamuiEnv *self, NwamuiSvc *svc)
{
	GtkTreeIter iter;
    NwamuiSvc *svcobj;
    nwam_error_t nerr;

    gtk_list_store_append (GTK_LIST_STORE(self->prv->svcs_model), &iter);
    gtk_list_store_set (GTK_LIST_STORE(self->prv->svcs_model), &iter,
      SVC_OBJECT, svcobj, -1);
   
    return TRUE;
}

extern NwamuiSvc*
nwamui_env_svc_add (NwamuiEnv *self, nwam_loc_prop_template_t svc)
{
	GtkTreeIter iter;
    NwamuiSvc *svcobj = NULL;
    nwam_error_t err;
    const char* fmri = NULL;
    
    if ( ( err = nwam_loc_prop_template_get_fmri( svc, &fmri )) == NWAM_SUCCESS ) {
        if ( fmri != NULL && (svcobj = nwamui_env_find_svc (self, fmri)) == NULL) {
            svcobj = nwamui_svc_new (svc);
            gtk_list_store_append (GTK_LIST_STORE(self->prv->svcs_model), &iter);
            gtk_list_store_set (GTK_LIST_STORE(self->prv->svcs_model), &iter,
              SVC_OBJECT, svcobj, -1);
        }
    }
    return svcobj;
}

extern void
nwamui_env_svc_add_full (NwamuiEnv *self,
  nwam_loc_prop_template_t svc,
  gboolean is_default,
  gboolean status)
{
    NwamuiSvc *svcobj;
    svcobj = nwamui_env_svc_add (self, svc);
    nwamui_svc_set_default (svcobj, is_default);
    nwamui_svc_set_status (svcobj, status);
    g_object_unref (svcobj);
}

static gboolean
nwamui_env_svc_commit (NwamuiEnv *self, NwamuiSvc *svc)
{
    nwam_error_t nerr = NWAM_INVALID_ARG;
    nwam_loc_prop_template_t *svc_h;
    
    /* FIXME: Can we actually add a service? */
    g_error("ERROR: Unable to add a service");
    
    g_object_get (svc, "nwam_svc", &svc_h, NULL);
    /* nerr = nwam_loc_svc_insert (self->prv->nwam_loc, &svc_h, 1); */
    return nerr == NWAM_SUCCESS;
}
#endif /* 0 */

/**
 * nwamui_env_has_modifications:   test if there are un-saved changes
 * @returns: TRUE if unsaved changes exist.
 **/
static gboolean
nwamui_object_real_has_modifications(NwamuiObject* object)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(object);

    return prv->nwam_loc_modified;
}

/**
 * nwamui_env_destroy:   destroy in-memory configuration, to persistant storage
 * @returns: TRUE if succeeded, FALSE if failed
 **/
static gboolean
nwamui_object_real_destroy( NwamuiObject *object )
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    nwam_error_t    nerr;

    g_return_val_if_fail( NWAMUI_IS_ENV(self), FALSE );

    if ( self->prv->nwam_loc != NULL ) {

        if ( (nerr = nwam_loc_destroy( self->prv->nwam_loc, 0 ) ) != NWAM_SUCCESS ) {
            g_warning("Failed when destroying LOC for %s", self->prv->name);
            return( FALSE );
        }
        self->prv->nwam_loc = NULL;
    }

    return( TRUE );
}

static gboolean
nwamui_object_real_is_modifiable(NwamuiObject *object)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(object);

    g_return_val_if_fail( NWAMUI_IS_ENV(object), FALSE );
    /* If Activation Mode is system, then you can't rename or remove
     * the selected location.
     */
    return (nwamui_object_real_get_activation_mode(object) != NWAMUI_COND_ACTIVATION_MODE_SYSTEM);
    /* return !NWAM_LOC_NAME_PRE_DEFINED(nwamui_object_get_name(object)); */
}

/**
 * nwamui_env_validate:   validate in-memory configuration
 * @prop_name_ret:  If non-NULL, the name of the property that failed will be
 *                  returned, should be freed by caller.
 * @returns: TRUE if valid, FALSE if failed
 */
static gboolean
nwamui_object_real_validate(NwamuiObject *object, gchar **prop_name_ret)
{
    NwamuiEnvPrivate *prv = NWAMUI_ENV_GET_PRIVATE(object);
    nwam_error_t    nerr;
    const char*     prop_name = NULL;

    g_return_val_if_fail( NWAMUI_IS_ENV(object), FALSE );

    if ( prv->nwam_loc_modified && prv->nwam_loc != NULL ) {
        nerr = nwam_loc_validate( prv->nwam_loc, &prop_name );
        if (nerr == NWAM_ENTITY_MULTIPLE_VALUES) {
            /* 7006036 */
            if ( prop_name_ret != NULL ) {
                *prop_name_ret = g_strdup_printf(_("%s and %s (mutally-exclusive)"),
                  NWAM_LOC_PROP_DNS_NAMESERVICE_DOMAIN,
                  NWAM_LOC_PROP_DNS_NAMESERVICE_SEARCH);
            }
            return( FALSE );
        } else if ( nerr != NWAM_SUCCESS ) {
            g_debug("Failed when validating LOC for %s : invalid value for %s", 
                    prv->name, prop_name);
            if ( prop_name_ret != NULL ) {
                *prop_name_ret = g_strdup( prop_name );
            }
            return( FALSE );
        }
    }

    return( TRUE );
}

/**
 * nwamui_env_commit:   commit in-memory configuration, to persistant storage
 * @returns: TRUE if succeeded, FALSE if failed
 **/
static gboolean
nwamui_object_real_commit( NwamuiObject *object )
{
    NwamuiEnv *self = NWAMUI_ENV(object);
    nwam_error_t    nerr;

    g_return_val_if_fail( NWAMUI_IS_ENV(self), FALSE );

    if ( self->prv->nwam_loc_modified && self->prv->nwam_loc != NULL ) {
        nwam_state_t                    state = NWAM_STATE_OFFLINE;
        nwam_aux_state_t                aux_state = NWAM_AUX_STATE_UNINITIALIZED;
        gboolean                        currently_enabled;

        if ( (nerr = nwam_loc_commit( self->prv->nwam_loc, 0 ) ) != NWAM_SUCCESS ) {
            g_warning("Failed when committing LOC for %s", self->prv->name);
            return( FALSE );
        }

        currently_enabled = get_nwam_loc_boolean_prop( self->prv->nwam_loc, NWAM_LOC_PROP_ENABLED );
        
        if ( self->prv->enabled != currently_enabled ) {
            /* Need to set enabled/disabled regardless of current state
             * since it's possible to switch from Automatic to Manual
             * selection, yet not change the active env, but we need to
             * ensure it's explicitly marked as enabled/disabled.
             */
            if ( self->prv->enabled ) {
                nwam_error_t nerr;
                if ( (nerr = nwam_loc_enable (self->prv->nwam_loc)) != NWAM_SUCCESS ) {
                    g_warning("Failed to enable location due to error: %s", nwam_strerror(nerr));
                    return (FALSE);
                }
            }
            else {
                /* Should only be done on a non-manual location if it's
                 * different to existing state.
                 */
                nwam_error_t nerr;
                if ( (nerr = nwam_loc_disable (self->prv->nwam_loc)) != NWAM_SUCCESS ) {
                    g_warning("Failed to disable location due to error: %s", nwam_strerror(nerr));
                    return (FALSE);
                }
            }
        }
        self->prv->nwam_loc_modified = FALSE;
    }

    return( TRUE );
}

static void
nwamui_env_finalize (NwamuiEnv *self)
{
    
    if (self->prv->name != NULL ) {
        g_free( self->prv->name );
    }

    if (self->prv->nwam_loc != NULL) {
        nwam_loc_free (self->prv->nwam_loc);
    }
    
#ifdef ENABLE_PROXY
    if (self->prv->proxy_pac_file != NULL ) {
        g_free( self->prv->proxy_pac_file );
    }

    if (self->prv->proxy_http_server != NULL ) {
        g_free( self->prv->proxy_http_server );
    }

    if (self->prv->proxy_https_server != NULL ) {
        g_free( self->prv->proxy_https_server );
    }

    if (self->prv->proxy_ftp_server != NULL ) {
        g_free( self->prv->proxy_ftp_server );
    }

    if (self->prv->proxy_gopher_server != NULL ) {
        g_free( self->prv->proxy_gopher_server );
    }

    if (self->prv->proxy_socks_server != NULL ) {
        g_free( self->prv->proxy_socks_server );
    }

    if (self->prv->proxy_bypass_list != NULL ) {
        g_free( self->prv->proxy_bypass_list );
    }

    if (self->prv->svcs_model != NULL ) {
        g_object_unref( self->prv->svcs_model );
    }

    if (self->prv->proxy_username != NULL ) {
        g_free( self->prv->proxy_username );
    }

    if (self->prv->proxy_password != NULL ) {
        g_free( self->prv->proxy_password );
    }
#endif /* ENABLE_PROXY */

    self->prv = NULL;

	G_OBJECT_CLASS(nwamui_env_parent_class)->finalize(G_OBJECT(self));
}

static nwam_state_t
nwamui_object_real_get_nwam_state(NwamuiObject *object, nwam_aux_state_t* aux_state_p, const gchar**aux_state_string_p)
{
    nwam_state_t    rstate = NWAM_STATE_UNINITIALIZED;

    if ( aux_state_p ) {
        *aux_state_p = NWAM_AUX_STATE_UNINITIALIZED;
    }

    if ( aux_state_string_p ) {
        *aux_state_string_p = (const gchar*)nwam_aux_state_to_string( NWAM_AUX_STATE_UNINITIALIZED );
    }

    if ( NWAMUI_IS_ENV( object ) ) {
        NwamuiEnv   *env = NWAMUI_ENV(object);
        nwam_state_t        state;
        nwam_aux_state_t    aux_state;


        /* First look at the LINK state, then the IP */
        if ( env->prv->nwam_loc &&
            nwam_loc_get_state( env->prv->nwam_loc, &state, &aux_state ) == NWAM_SUCCESS ) {

            rstate = state;
            if ( aux_state_p ) {
                *aux_state_p = aux_state;
            }
            if ( aux_state_string_p ) {
                *aux_state_string_p = (const gchar*)nwam_aux_state_to_string( aux_state );
            }
        }
    }

    return(rstate);
}

/* Callbacks */

static void 
svc_row_inserted_or_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    NwamuiNcu* self = NWAMUI_NCU(data);

    /* Chain events in the tree to the NCU object*/
    g_object_notify(G_OBJECT(self), "svcs_model");
}

static void 
svc_row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path, gpointer data)
{
    NwamuiNcu* self = NWAMUI_NCU(data);

    /* Chain events in the tree to the NCU object*/
    g_object_notify(G_OBJECT(self), "svcs_model");
}

#if 0
/* These are not needed right now since we don't support property templates,
 * but would like to keep around for when we do.
 */
/* walkers */
static int
nwam_loc_svc_walker_cb (nwam_loc_prop_template_t svc, void *data)
{
    NwamuiEnv* self = NWAMUI_ENV(data);
    NwamuiEnvPrivate* prv = NWAMUI_ENV(data)->prv;

    /* TODO: status, is default? */
    nwamui_env_svc_add_full (self, svc, TRUE, TRUE);
}

static int
nwam_loc_sys_svc_walker_cb (nwam_loc_handle_t env, nwam_loc_prop_template_t svc, void *data)
{
    NwamuiEnv* self = NWAMUI_ENV(data);
    NwamuiEnvPrivate* prv = NWAMUI_ENV(data)->prv;

    /* TODO: status, is default? */
	GtkTreeIter iter;
    NwamuiSvc *svcobj;

    svcobj = nwamui_svc_new (svc);
    gtk_list_store_append (GTK_LIST_STORE(self->prv->sys_svcs_model), &iter);
    gtk_list_store_set (GTK_LIST_STORE(self->prv->sys_svcs_model), &iter,
      SVC_OBJECT, svcobj, -1);
    g_object_unref (svcobj);
}
#endif /* 0 */

const gchar*
nwam_nameservices_enum_to_string(nwam_nameservices_t ns)
{
    switch (ns) {
	case NWAM_NAMESERVICES_DNS:
        return _("DNS");
	case NWAM_NAMESERVICES_FILES:
        return _("FILES");
	case NWAM_NAMESERVICES_NIS:
        return _("NIS");
	case NWAM_NAMESERVICES_LDAP:
        return _("LDAP");
    default:
        return NULL;
    }
}

const gchar*
nwamui_env_nameservices_enum_to_filename(nwamui_env_nameservices_t ns)
{
    switch (ns) {
	case NWAMUI_ENV_NAMESERVICES_DNS:
        return (SYSCONFDIR "/nsswitch.dns");
	case NWAMUI_ENV_NAMESERVICES_FILES:
        return (SYSCONFDIR "/nsswitch.files");
	case NWAMUI_ENV_NAMESERVICES_NIS:
        return (SYSCONFDIR "/nsswitch.nis");
	case NWAMUI_ENV_NAMESERVICES_LDAP:
        return (SYSCONFDIR "/nsswitch.ldap");
    default:
        return NULL;
    }
}
