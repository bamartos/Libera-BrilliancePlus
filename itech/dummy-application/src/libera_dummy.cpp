/*
 * Copyright (c) 2008-2012 Instrumentation Technologies
 * All Rights Reserved.
 *
 * $Id: libera_dummy.cpp 18382 2012-12-21 13:09:48Z tomaz.beltram $
 */

#include <isig/signal_source.h>
#include <isig/random_access_file.h>

#include <ireg/node.h>
#include <ireg/reg_node.h>
#include <ireg/remote_node.h>
#include <ireg/signal_node.h>
#include <ireg/reg_value_node.h>
#include <ireg/reg_reference_node.h>
#include <ireg/reg_reference_enum_node.h>
#include <ireg/reg_fnc_node.h>
#include <ireg/reg_value_enum_node.h>
#include <ireg/reg_fnc_enum_node.h>
#include <ireg/reg_config_node.h>
#include <ireg/reg_reference_bit_node.h>
#include <ireg/reg_exec_node.h>
#include "ireg/expression_operations.h"
#include "ireg/expression_terminals.h"

#include <ireg/ireg_util.h>

#include "libera_dummy.h"
#include "simul_stream.h"

static const std::string c_applicationName("libera-dummy");

enum TbtSigSel_e {
    eInternal       = 0,
    eExternal       = 1
};

template <>
const istd::EnumMap<TbtSigSel_e>::Items istd::EnumMap<TbtSigSel_e>::items =
    {{eInternal, "Internal"},
     {eExternal, "External"}};

enum tAppliedCoefficients   { eUnity, eAdjusted };

template <>
const istd::EnumMap<tAppliedCoefficients>::Items istd::EnumMap<tAppliedCoefficients>::items =
            {{eUnity,    "unity"},
             {eAdjusted, "adjusted"}};

/*-----------------------------------------------------------------------------*/
/* return application name                                                     */

const std::string& liberaDummy::ApplicationName() const {
	return c_applicationName;
}

/*-----------------------------------------------------------------------------*/
/* constructor                                                                 */

liberaDummy::liberaDummy() : LiberaApplication(c_applicationName)
{
	istd::TraceFile::GetInstance().SetPrintFile(true);
	istd::TraceFile::GetInstance().SetDumpStack(true);

	m_exitThread1 = false;
}

/*-----------------------------------------------------------------------------*/
/* destructor                                                                  */

liberaDummy::~liberaDummy()
{
	istd_FTRC();
}

/*-----------------------------------------------------------------------------*/
/* virtual event handler, triggered after all boards are instantiated and      */
/* before registry is defined                                                  */

void liberaDummy::OnPreInit()
{
	istd_FTRC();
}

/*-----------------------------------------------------------------------------*/
/* virtual event handler, triggered after all boards are instantiated and      */
/* registry is defined                                                         */

void liberaDummy::OnPostInit()
{
	istd_FTRC();

    // MCI registry is ready to be used from now on
    mci::Node root = GetRegistry();

	// Trace info - what is expored by platform daemon
	istd_TRC(istd::eTrcLow,
        "Exporting following nodes through the ireg:"
        << std::endl << root.DumpSubTree()
    );

}

/*-----------------------------------------------------------------------------*/
/* virtual event thread that sets values to some registry nodes to see the     */
/* changes and notifications appear                                            */

void liberaDummy::EventThread()
{
    istd_FTRC();

    while (1) {
    	if (m_exitThread1) {
    		break;
    	}
    	sleep(1);
    }
}

/*-----------------------------------------------------------------------------*/
/* virtual event handler, used for application initialization after            */
/* registry is updated with persistent configuration.                          */

void liberaDummy::OnPostConfig() {
	istd_FTRC();

	m_thread1 = std::thread(std::bind(&liberaDummy::EventThread, this));
}

void liberaDummy::RegistryAddTiming(const char *a_name)
{
    istd_FTRC();

    mci::Node boards = GetRegistry()["boards"];
    boards.GetTreeNode()
        ->Attach(Create<RegNode>(a_name)
            ->Attach(Create<RegNode>("pll")
                ->Attach(Create<RegValueBoolNode>("locked", true))
                ->Attach(Create<RegValueInt32Node>("vcxo_offset", 100))
                ->Attach(Create<RegValueBoolNode>("compensate_offset", true))
            )
            ->Attach(Create<RegNode>("events")
                ->Attach(Create<RegNode>("t1")
                    ->Attach(Create<RegValueUInt64Node>("count", 123))
                )
                ->Attach(Create<RegNode>("t2")
                    ->Attach(Create<RegValueUInt64Node>("count", 345))
                )
            )
        );
}

bool liberaDummy::IlkStatusReset()
{
    istd_FTRC();
    m_ilkStatus = 0;
    return true;
}

bool liberaDummy::RegExec()
{
    istd_FTRC();
}

void liberaDummy::RegistryAddRaf(const char *a_name)
{
    istd_FTRC();

    auto sa = std::make_shared<SaStream>();
    m_signals.emplace_back(std::static_pointer_cast<isig::SignalSource>(sa));

    auto adc = std::make_shared<SimulAdc>();
    m_signals.emplace_back(std::static_pointer_cast<isig::SignalSource>(adc));

    auto ddc_raw = std::make_shared<SimulDdcRaw>();
    m_signals.emplace_back(std::static_pointer_cast<isig::SignalSource>(ddc_raw));

    auto ddc_synth = std::make_shared<SimulDdcSynth>();
    m_signals.emplace_back(std::static_pointer_cast<isig::SignalSource>(ddc_synth));

    auto pm_synth = std::make_shared<SimulDdcSynth>();
    m_signals.emplace_back(std::static_pointer_cast<isig::SignalSource>(pm_synth));

    mci::Node boards = GetRegistry()["boards"];
    boards.GetTreeNode()
        ->Attach(Create<RegNode>(a_name)
            ->Attach(Create<RegNode>("conditioning")
                ->Attach(Create<RegNode>("tuning")
                    ->Attach(Create<RegNode>("agc")
                        ->Attach(Create<RegValueBoolNode>("enabled", true))
                        ->Attach(Create<RegValueInt32Node>("power_level", -66))
                    )
                    ->Attach(Create<RegNode>("dsc")
                        ->Attach(Create<RegNode>("coefficients")
                            ->Attach(Create<RegValueBoolNode>("adjust", false))
                            ->Attach(Create<RegValueEnumNode<tAppliedCoefficients> >("type", eUnity))
                            ->Attach(Create<RegExecNode>("store", this, &liberaDummy::RegExec))
                        )
                    )
                )
                ->Attach(Create<RegValueBoolNode>("switching", true))
            )
            ->Attach(Create<RegNode>("conf")
                ->Attach(Create<RegValueUInt32Node>("switching_delay", 0))
                ->Attach(Create<RegValueEnumNode<TbtSigSel_e> >("switching_source", eInternal))
            )
            ->Attach(Create<RegNode>("tbt")
                ->Attach(Create<RegValueUInt32Node>("phase_offset", 0))
            )
            ->Attach(Create<RegNode>("local_timing")
                ->Attach(Create<RegValueUInt32Node>("trigger_delay", 0))
            )
            ->Attach(Create<RegNode>("interlock")
                ->Attach(Create<RegValueBoolNode>("enabled", false))
                ->Attach(Create<RegNode>("gain_dependent")
                    ->Attach(Create<RegValueBoolNode>("enabled", false))
                    ->Attach(Create<RegValueInt32Node>("threshold", -40))
                )
                ->Attach(Create<RegNode>("limits")
                    ->Attach(Create<RegNode>("position")
                        ->Attach(Create<RegNode>("min")
                            ->Attach(Create<RegValueInt32Node>("x", -1000000))
                            ->Attach(Create<RegValueInt32Node>("y", -1000000))
                        )
                        ->Attach(Create<RegNode>("max")
                            ->Attach(Create<RegValueInt32Node>("x", 1000000))
                            ->Attach(Create<RegValueInt32Node>("y", 1000000))
                        )
                    )
                    ->Attach(Create<RegNode>("overflow")
                        ->Attach(Create<RegValueUInt32Node>("threshold", 30000))
                        ->Attach(Create<RegValueUInt32Node>("duration", 5))
                    )
                )
                ->Attach(Create<RegNode>("status")
                    ->Attach(Create<RegNode>("il_status")
                        ->Attach(Create<RegExecNode>("reset", this, &liberaDummy::IlkStatusReset))
                        ->Attach(Create<RegRefUint64VBoolNode>("x",                     m_ilkStatus, 0, 1))
                        ->Attach(Create<RegRefUint64VBoolNode>("y",                     m_ilkStatus, 1, 1))
                        ->Attach(Create<RegRefUint64VBoolNode>("attenuator",            m_ilkStatus, 2, 1))
                        ->Attach(Create<RegRefUint64VBoolNode>("adc_overflow_filtered", m_ilkStatus, 3, 1))
                        ->Attach(Create<RegRefUint64VBoolNode>("adc_overflow",          m_ilkStatus, 4, 1))
                    )
                )
            )
            ->Attach(Create<RegNode>("postmortem")
                ->Attach(Create<RegValueBoolNode>("capture", false))
                ->Attach(Create<RegValueInt32Node>("offset", 0))
                ->Attach(Create<RegNode>("signals")
                    ->Attach(Create<SignalNode>("ddc_synthetic", pm_synth.get()))
                )
            )
            ->Attach(Create<RegNode>("signal_processing")
                ->Attach(Create<RegNode>("position")
                    ->Attach(Create<RegValueUInt32Node>("Kx", 10000000))
                    ->Attach(Create<RegValueUInt32Node>("Ky", 10000000))
                    ->Attach(Create<RegValueInt32Node>("off_x", 0))
                    ->Attach(Create<RegValueInt32Node>("off_y", 0))
                )
            )
            ->Attach(Create<RegNode>("events")
                ->Attach(Create<RegValueInt32Node>("trigger", 123456))
             )
            ->Attach(Create<RegNode>("signals")
                ->Attach(Create<SignalNode>("adc", adc.get()))
                ->Attach(Create<SignalNode>("ddc_raw", ddc_raw.get()))
                ->Attach(Create<SignalNode>("ddc_synthetic", ddc_synth.get()))
                ->Attach(Create<SignalNode>("sa", sa.get()))
            )
        );
}

/*-----------------------------------------------------------------------------*/
/* expose our functionality to outside world                                   */

void liberaDummy::OnRegistryAdd(ireg::TreeNodePtr &parent) {
	istd_FTRC();

    RegistryAddTiming("evrx2");
    //TODO: for raf3..6
	RegistryAddRaf("raf5");
}

/*-----------------------------------------------------------------------------*/
/* virtual event handler, used for application specific shutdown procedure     */

void liberaDummy::OnShutdown() {
	istd_FTRC();

	m_exitThread1 = true;

    if (m_thread1.joinable()) {
       m_thread1.join();
    }

    for (auto i(m_signals.begin()); i != m_signals.end(); ++i) {
        (*i)->ShutDown();
    }
}

/*-----------------------------------------------------------------------------*/
/* no boards to create here */

iapp::LiberaBoard *liberaDummy::CreateBoard(const bmc::Board* a_board) {
	istd_FTRC();

	return NULL;
}

/*-----------------------------------------------------------------------------*/
/* Instantiate iapp:LiberaApplivarion in main()                                */

int main(int argc, char *argv[])
{
    auto ld( std::make_shared<liberaDummy>() );
    bool success = ld->Run(argc, argv);
    return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

