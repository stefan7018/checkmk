// Copyright (C) 2019 tribe29 GmbH - License: GNU General Public License v2
// This file is part of Checkmk (https://checkmk.com). It is subject to the
// terms and conditions defined in the file COPYING, which is part of this
// source code package.

#include "auth.h"

#include "contact_fwd.h"

namespace {
bool host_has_contact(const host *hst, const contact *ctc) {
    // Older Nagios headers are not const-correct... :-P
    return is_contact_for_host(const_cast<host *>(hst),
                               const_cast<contact *>(ctc)) != 0 ||
           is_escalated_contact_for_host(const_cast<host *>(hst),
                                         const_cast<contact *>(ctc)) != 0;
}

bool service_has_contact(ServiceAuthorization service_auth, const service *svc,
                         const contact *ctc) {
    // Older Nagios headers are not const-correct... :-P
    return is_contact_for_service(const_cast<service *>(svc),
                                  const_cast<contact *>(ctc)) != 0 ||
           is_escalated_contact_for_service(const_cast<service *>(svc),
                                            const_cast<contact *>(ctc)) != 0 ||
           (service_auth == ServiceAuthorization::loose &&
            host_has_contact(svc->host_ptr, ctc));
}
}  // namespace

bool is_authorized_for_hst(const contact *ctc, const host *hst) {
    if (ctc == nullptr) {
        return true;
    }
    if (ctc == unknown_auth_user()) {
        return false;
    }
    return host_has_contact(hst, ctc);
}

bool is_authorized_for_svc(ServiceAuthorization service_auth,
                           const contact *ctc, const service *svc) {
    if (ctc == nullptr) {
        return true;
    }
    if (ctc == unknown_auth_user()) {
        return false;
    }
    return service_has_contact(service_auth, svc, ctc);
}

bool is_authorized_for_host_group(GroupAuthorization group_auth,
                                  const hostgroup *hg, const contact *ctc) {
    if (ctc == nullptr) {
        return true;
    }
    if (ctc == unknown_auth_user()) {
        return false;
    }

    auto has_contact = [=](hostsmember *mem) {
        return is_authorized_for_hst(ctc, mem->host_ptr);
    };
    if (group_auth == GroupAuthorization::loose) {
        // TODO(sp) Need an iterator here, "loose" means "any_of"
        for (hostsmember *mem = hg->members; mem != nullptr; mem = mem->next) {
            if (has_contact(mem)) {
                return true;
            }
        }
        return false;
    }
    // TODO(sp) Need an iterator here, "strict" means "all_of"
    for (hostsmember *mem = hg->members; mem != nullptr; mem = mem->next) {
        if (!has_contact(mem)) {
            return false;
        }
    }
    return true;
}

bool is_authorized_for_service_group(GroupAuthorization group_auth,
                                     ServiceAuthorization service_auth,
                                     const servicegroup *sg,
                                     const contact *ctc) {
    if (ctc == nullptr) {
        return true;
    }
    if (ctc == unknown_auth_user()) {
        return false;
    }

    auto has_contact = [=](servicesmember *mem) {
        return is_authorized_for_svc(service_auth, ctc, mem->service_ptr);
    };
    if (group_auth == GroupAuthorization::loose) {
        // TODO(sp) Need an iterator here, "loose" means "any_of"
        for (servicesmember *mem = sg->members; mem != nullptr;
             mem = mem->next) {
            if (has_contact(mem)) {
                return true;
            }
        }
        return false;
    }
    // TODO(sp) Need an iterator here, "strict" means "all_of"
    for (servicesmember *mem = sg->members; mem != nullptr; mem = mem->next) {
        if (!has_contact(mem)) {
            return false;
        }
    }
    return true;
}
