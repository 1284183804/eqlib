#pragma once
#include "yyh.h"
#include "gglx.h"
#include "jhqhx.h"
#include "dtjhq.h"
#include "cl.h"
#include "zc.h"
#include "ppfx.h"
#include "zdjhq.h"
#include "zdzy.h"
#include "bh.h"
#include "dtd.h"
#include "wjdx.h"
#include "nccwj.h"
#include "xxwjhq.h"
#include "hhxw.h"
#include "caiyang.h"
#include <vector>

struct EQLIB_INTERNAL eqlib_handle {
    double                        sample_rate{48000.0};
    int                           channels{2};
    int                           buffer_frames{4096};

    eqlib::EqCore                 core;
    eqlib::MultiBandDynEq         multiband_eq;
    eqlib::SidechainProcessor     sidechain;
    eqlib::MidSideProcessor       mid_side;
    eqlib::SpectrumAnalyzer       spectrum;
    eqlib::AutoEqCore             auto_eq;
    eqlib::AgcCore                agc;
    eqlib::SaturationCore         saturation;
    eqlib::MultiChannelProcessor  multichannel;
    eqlib::WavWriter              wav_writer;
    eqlib::WavReader              wav_reader;
    eqlib::MemoryWavWriter        mem_writer;
    eqlib::MemoryWavReader        mem_reader;
    eqlib::LinearPhaseEq          linear_phase;
    eqlib::HybridPhaseEq          hybrid_phase;
    eqlib::Resampler              resampler;

    std::vector<float>            audio_buffer_f;
    std::vector<double>           audio_buffer_d;
    bool                          audio_buffer_d_valid{false};
    eqlib::AudioFileInfo          loaded_info{};
};
