/*
 * JBoss, Home of Professional Open Source
 * Copyright 2009, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#ifndef _TXIORINTERCEPTOR_H
#define _TXIORINTERCEPTOR_H

#include "tao/PortableInterceptorC.h"
#include "tao/LocalObject.h"
#include "tao/IORInterceptor_Adapter.h"
#include "tao/IORInterceptor/IORInterceptor.h"

#include "TxInterceptor.h"

/**
 * IOR interceptor for tagging IORs as transactional
 */
class TxIORInterceptor:
        public virtual PortableInterceptor::IORInterceptor,
        public TxInterceptor
{
public:
        TxIORInterceptor(const char *, IOP::CodecFactory_var);
        virtual ~TxIORInterceptor() {}

        char* name() { return TxInterceptor::name();}
        void destroy() { TxInterceptor::destroy();}

        void establish_components(PortableInterceptor::IORInfo_ptr info);

        void components_established (PortableInterceptor::IORInfo_ptr) {}
        void adapter_manager_state_changed(PortableInterceptor::AdapterManagerId, PortableInterceptor::AdapterState) {}
        void adapter_state_changed(const PortableInterceptor::ObjectReferenceTemplateSeq&, PortableInterceptor::AdapterState) {}

private:
        void addOTSTag(PortableInterceptor::IORInfo_ptr info);
};
#endif //_TXIORINTERCEPTOR_H
