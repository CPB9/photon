#pragma once

#include "photon/Config.hpp"

#include <caf/atom.hpp>

namespace photon {

using StartAtom                           = caf::atom_constant<caf::atom("startact")>;
using StopAtom                            = caf::atom_constant<caf::atom("sstopact")>;
using LogAtom                             = caf::atom_constant<caf::atom("logevent")>;
using RecvDataAtom                        = caf::atom_constant<caf::atom("recvdata")>;
using RecvPayloadAtom                     = caf::atom_constant<caf::atom("recvpyld")>;
using EnableLoggindAtom                   = caf::atom_constant<caf::atom("enablelg")>;
using RecvPacketPayloadAtom               = caf::atom_constant<caf::atom("recvupkt")>;
using SendUnreliablePacketAtom            = caf::atom_constant<caf::atom("sendupkt")>;
using SendReliablePacketAtom              = caf::atom_constant<caf::atom("sendrpkt")>;
using SendFwtPacketAtom                   = caf::atom_constant<caf::atom("sendupkt")>;
using SendGcCommandAtom                   = caf::atom_constant<caf::atom("sendgccm")>;
using SetProjectAtom                      = caf::atom_constant<caf::atom("setprojt")>;
using RegisterClientAtom                  = caf::atom_constant<caf::atom("regclien")>;
using SetTmViewAtom                       = caf::atom_constant<caf::atom("settmviw")>;
using UpdateFirmware                      = caf::atom_constant<caf::atom("updfirmw")>;
using UpdateTmViewAtom                    = caf::atom_constant<caf::atom("upsttmvi")>;
using UpdateTmParams                      = caf::atom_constant<caf::atom("updtmprm")>;
using PushTmUpdatesAtom                   = caf::atom_constant<caf::atom("pshtmupd")>;
using SubscribeNamedTmAtom                = caf::atom_constant<caf::atom("subsnatm")>;
using SubscribeNumberedTmAtom             = caf::atom_constant<caf::atom("subsnutm")>;
using SendCustomCommandAtom               = caf::atom_constant<caf::atom("sendccmd")>;
using PingAtom                            = caf::atom_constant<caf::atom("pingatom")>;

using RepeatStreamAtom                    = caf::atom_constant<caf::atom("strmrept")>;
using SetStreamDestAtom                   = caf::atom_constant<caf::atom("strmdest")>;

using FwtHashAtom                         = caf::atom_constant<caf::atom("fwthash")>;
using FwtStartAtom                        = caf::atom_constant<caf::atom("fwtstart")>;
using FwtCheckAtom                        = caf::atom_constant<caf::atom("fwtcheck")>;

using ExchangeErrorEventAtom              = caf::atom_constant<caf::atom("excerror")>;

using FirmwareDownloadStartedEventAtom    = caf::atom_constant<caf::atom("feventbgn")>;
using FirmwareDownloadFinishedEventAtom   = caf::atom_constant<caf::atom("feventfin")>;
using FirmwareSizeRecievedEventAtom       = caf::atom_constant<caf::atom("feventsiz")>;
using FirmwareStartCmdSentEventAtom       = caf::atom_constant<caf::atom("feventscs")>;
using FirmwareStartCmdPassedEventAtom     = caf::atom_constant<caf::atom("feventscp")>;
using FirmwareHashDownloadedEventAtom     = caf::atom_constant<caf::atom("feventhok")>;
using FirmwareErrorEventAtom              = caf::atom_constant<caf::atom("feventerr")>;
using FirmwareProgressEventAtom           = caf::atom_constant<caf::atom("feventprg")>;

using RequestDfuStatus                    = caf::atom_constant<caf::atom("reqsfusta")>;
using FlashDfuFirmware                    = caf::atom_constant<caf::atom("flashdfuf")>;

}
