/* modify.c - monitor backend modify routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2005 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-monitor.h"
#include "proto-back-monitor.h"

int
monitor_back_modify( Operation *op, SlapReply *rs )
{
	int 		rc = 0;
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	Entry		*matched;
	Entry		*e;

	Debug(LDAP_DEBUG_ARGS, "monitor_back_modify:\n", 0, 0, 0);

	/* acquire and lock entry */
	monitor_cache_dn2entry( op, &op->o_req_ndn, &e, &matched );
	if ( e == NULL ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		if ( matched ) {
#ifdef SLAP_ACL_HONOR_DISCLOSE
			if ( !access_allowed_mask( op, matched,
					slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL, NULL ) )
			{
				/* do nothing */ ;
			} else 
#endif /* SLAP_ACL_HONOR_DISCLOSE */
			{
				rs->sr_matched = matched->e_dn;
			}
		}
		send_ldap_result( op, rs );
		if ( matched != NULL ) {
			rs->sr_matched = NULL;
			monitor_cache_release( mi, matched );
		}
		return rs->sr_err;
	}

	if ( !acl_check_modlist( op, e, op->oq_modify.rs_modlist )) {
		rc = LDAP_INSUFFICIENT_ACCESS;

	} else {
		rc = monitor_entry_modify( op, e );
	}

#ifdef SLAP_ACL_HONOR_DISCLOSE
	if ( rc != LDAP_SUCCESS ) {
		if ( !access_allowed_mask( op, e, slap_schema.si_ad_entry,
				NULL, ACL_DISCLOSE, NULL, NULL ) )
		{
			rc = LDAP_NO_SUCH_OBJECT;
		}
	}
#endif /* SLAP_ACL_HONOR_DISCLOSE */

	rs->sr_err = rc;
	send_ldap_result( op, rs );

	monitor_cache_release( mi, e );

	return rs->sr_err;
}

