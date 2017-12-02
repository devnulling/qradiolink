// Written by Adrian Musceac YO8RZZ , started March 2016.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "gr_demod_wbfm_sdr.h"

gr_demod_wbfm_sdr_sptr make_gr_demod_wbfm_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_wbfm_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_wbfm_sdr::gr_demod_wbfm_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_wbfm_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{

    _target_samp_rate = 200000;

    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;


    float rerate = 8000.0/(float)_target_samp_rate;

    unsigned int flt_size = 32;

    static const float coeff[] = {8.23112713987939e-05, 0.30221322178840637,
                                  1.3954089879989624, 0.302213191986084, 8.23112713987939e-05};
    std::vector<float> iir_taps(coeff, coeff + sizeof(coeff) / sizeof(coeff[0]) );
    _deemphasis_filter = gr::filter::fft_filter_fff::make(1,iir_taps);

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _filter_width, 12000);
    std::vector<float> audio_taps = gr::filter::firdes::low_pass(1, _target_samp_rate, 4000, 600);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1,5,taps);
    _audio_resampler = gr::filter::pfb_arb_resampler_fff::make(rerate, audio_taps, flt_size);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                            1, _target_samp_rate, _filter_width,600,gr::filter::firdes::WIN_HAMMING) );

    _pilot_filter = gr::filter::fft_filter_fff::make(1,gr::filter::firdes::low_pass(
                                                         1,_target_samp_rate,4000,
                                                         600,gr::filter::firdes::WIN_HAMMING));
    _fm_demod = gr::analog::quadrature_demod_cf::make(_target_samp_rate/(2*M_PI* _filter_width));
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _amplify = gr::blocks::multiply_const_ff::make(10);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_squelch,0);
    connect(_squelch,0,_fm_demod,0);
    connect(_fm_demod,0,_pilot_filter,0);
    connect(_pilot_filter,0,_amplify,0);
    connect(_amplify,0,_audio_resampler,0);
    connect(_audio_resampler,0,self(),1);


}


void gr_demod_wbfm_sdr::set_squelch(int value)
{
    _squelch->set_threshold(value);
}