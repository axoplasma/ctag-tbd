/***************
CTAG TBD >>to be determined<< is an open source eurorack synthesizer module.

A project conceived within the Creative Technologies Arbeitsgruppe of
Kiel University of Applied Sciences: https://www.creative-technologies.de

(c) 2020 by Robert Manzke. All rights reserved.

The CTAG TBD software is licensed under the GNU General Public License
(GPL 3.0), available here: https://www.gnu.org/licenses/gpl-3.0.txt

The CTAG TBD hardware design is released under the Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0).
Details here: https://creativecommons.org/licenses/by-nc-sa/4.0/

CTAG TBD is provided "as is" without any express or implied warranties.

License and copyright details for specific submodules are included in their
respective component folders / files if different from this license.
***************/

#include <atomic>
#include "ctagSoundProcessor.hpp"
#include "helpers/ctagSampleRom.hpp"
#include "helpers/ctagSineSource.hpp"
#include "helpers/ctagADSREnv.hpp"
#include "plaits/dsp/oscillator/wavetable_oscillator.h"
#include "braids/quantizer.h"
#include "stmlib/dsp/filter.h"
#include "helpers/ctagRollingAverage.hpp"

using namespace CTAG::SP::HELPERS;

namespace CTAG {
    namespace SP {
        class ctagSoundProcessorWTOscDuo : public ctagSoundProcessor {
        public:
            virtual void Process(const ProcessData &) override;
           virtual void Init() override;
            virtual ~ctagSoundProcessorWTOscDuo();

        private:
            virtual void knowYourself() override;
            void prepareWavetables();
            ctagSampleRom sample_rom_1;
            ctagSampleRom sample_rom_2;
            plaits::WavetableOscillator<256, 64> oscillator_1;
            plaits::WavetableOscillator<256, 64> oscillator_2;
            ctagSineSource lfo_1;
            ctagSineSource lfo_2;
            ctagADSREnv adsr_1;
            ctagADSREnv adsr_2;
            stmlib::Svf svf_1;
            stmlib::Svf svf_2;
            int16_t *buffer = NULL;
            float *fbuffer = NULL;
            const int16_t *wavetables[64];
            int currentBank = 0;
            int lastBank = -1;
            bool isWaveTableGood = false;
            float valADSR_1 = 0.f;
            float valADSR_2 = 0.f; 
            float valLFO_1 = 0.f;
            float valLFO_2 = 0.f;
            bool preGate_1 = false;
            bool preGate_2 = false;
            braids::Quantizer pitchQuantizer;
            float pre_fWt_1 = 0.f;
            float pre_fWt_2 = 0.f;
            // private attributes could go here
            // autogenerated code here
            // sectionHpp
	atomic<int32_t> gain_1, cv_gain_1;
	atomic<int32_t> gain_2, cv_gain_2;
	atomic<int32_t> gate_1, trig_gate_1;
	atomic<int32_t> pitch_1, cv_pitch_1;
	atomic<int32_t> gate_2, trig_gate_2;
	atomic<int32_t> pitch_2, cv_pitch_2;
	atomic<int32_t> q_scale, cv_q_scale;
	atomic<int32_t> tune_1, cv_tune_1;
	atomic<int32_t> tune_2, cv_tune_2;
	atomic<int32_t> wavebank, cv_wavebank;
	atomic<int32_t> wave_1, cv_wave_1;
	atomic<int32_t> wave_2, cv_wave_2;
	atomic<int32_t> fmode, cv_fmode;
	atomic<int32_t> fcut_1, cv_fcut_1;
	atomic<int32_t> fcut_2, cv_fcut_2;
	atomic<int32_t> freso, cv_freso;
	atomic<int32_t> lfo2wave, cv_lfo2wave;
	atomic<int32_t> lfo2am, cv_lfo2am;
	atomic<int32_t> lfo2fm, cv_lfo2fm;
	atomic<int32_t> lfo2filtfm, cv_lfo2filtfm;
	atomic<int32_t> eg2wave, cv_eg2wave;
	atomic<int32_t> eg2am, cv_eg2am;
	atomic<int32_t> eg2fm, cv_eg2fm;
	atomic<int32_t> eg2filtfm, cv_eg2filtfm;
	atomic<int32_t> lfospeed, cv_lfospeed;
	atomic<int32_t> vintage_vibe, cv_vintage_vibe;
	atomic<int32_t> lfosync_1, trig_lfosync_1;
	atomic<int32_t> lfosync_2, trig_lfosync_2;
	atomic<int32_t> egfasl, trig_egfasl;
	atomic<int32_t> attack, cv_attack;
	atomic<int32_t> decay, cv_decay;
	atomic<int32_t> sustain, cv_sustain;
	atomic<int32_t> release, cv_release;
	// sectionHpp
        };
    }
}