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
 * File:   nwamui_ncp.h
 *
 */

#ifndef _NWAMUI_NCP_H
#define	_NWAMUI_NCP_H

#ifndef _libnwamui_H
#error "Please include libnwamui.h header instead."
#endif

G_BEGIN_DECLS

#define NWAMUI_TYPE_NCP               (nwamui_ncp_get_type ())
#define NWAMUI_NCP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), NWAMUI_TYPE_NCP, NwamuiNcp))
#define NWAMUI_NCP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), NWAMUI_TYPE_NCP, NwamuiNcpClass))
#define NWAMUI_IS_NCP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NWAMUI_TYPE_NCP))
#define NWAMUI_IS_NCP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), NWAMUI_TYPE_NCP))
#define NWAMUI_NCP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), NWAMUI_TYPE_NCP, NwamuiNcpClass))


typedef struct _NwamuiNcp	      NwamuiNcp;
typedef struct _NwamuiNcpClass        NwamuiNcpClass;
typedef struct _NwamuiNcpPrivate      NwamuiNcpPrivate;

struct _NwamuiNcp
{
	GObject                      object;

	/*< private >*/
	NwamuiNcpPrivate            *prv;
};

struct _NwamuiNcpClass
{
	GObjectClass                parent_class;
};



extern  GType                   nwamui_ncp_get_type (void) G_GNUC_CONST;

/* For Phase 1 there is only one instance, so using a singleton, but 
 * for Phase 2+, with multiple instances, we would use the _new function.
 *
 * extern  NwamuiNcp*              nwamui_ncp_new (void);
 *
 */
extern  NwamuiNcp*              nwamui_ncp_get_instance (void);

extern  gchar*                  nwamui_ncp_get_name ( NwamuiNcp *self );

extern	GList*                  nwamui_ncp_get_ncu_list ( NwamuiNcp *self );

extern  void                    nwamui_ncp_foreach_ncu( NwamuiNcp *self, GFunc func, gpointer user_data );
G_END_DECLS

#endif	/* _NWAMUI_NCP_H */

