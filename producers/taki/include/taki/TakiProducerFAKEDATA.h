 /***************************************************************************
 *   Copyright (C) 2007 by Chair for circuitry and simulation              *
 *                         University of Mannheim, Germany                 *
 *                         www.ziti.uni-heidelberg.de                      *
 *                                                                         *
 *   Author: Takacs, Corrin                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TakiProducerFAKEDATA_H
#define TakiProducerFAKEDATA_H

#include "eudaq/Producer.hh"

class TakiProducerFAKEDATA : public eudaq::Producer
{

    public:
        TakiProducerFAKEDATA(const std::string & name, const std::string & runcontrol);
        void send(const char* const p_uc_buffer, const int& r_i_numBytes, const unsigned long long& ull_triggerID);
        void sendCorrupt(const char* const p_uc_buffer, const int& r_i_numBytes, const unsigned long long& ull_triggerID);
        virtual void OnConfigure(const eudaq::Configuration & param);
        virtual void OnStartRun(unsigned param);
        virtual void OnStopRun();
        virtual void OnTerminate();
        virtual void OnReset();
        virtual void OnStatus();
        virtual void OnUnrecognised(const std::string & cmd, const std::string & param);
        virtual bool runInProgress();
        virtual bool isDone();

        bool m_b_oldRunJustEnded;

    private:
        bool     m_b_done, m_b_runInProgress;
        unsigned m_u_run, m_u_ev, m_u_eventsize;

};

#endif //TakiProducerFAKEDATA_H
