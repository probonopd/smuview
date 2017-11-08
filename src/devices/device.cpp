/*
 * This file is part of the SmuView project.
 *
 * Copyright (C) 2015 Joel Holdsworth <joel@airwebreathe.org.uk>
 * Copyright (C) 2017 Frank Stettner <frank-stettner@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>

#include <libsigrokcxx/libsigrokcxx.hpp>

#include <QDateTime>
#include <QDebug>

#include "device.hpp"
#include "src/data/analog.hpp"
#include "src/data/signalbase.hpp"
#include "src/data/signaldata.hpp"

using std::bad_alloc;
using std::dynamic_pointer_cast;
using std::lock_guard;
using std::make_shared;
using std::pair;
using std::set;
using std::shared_ptr;
using std::static_pointer_cast;
using std::string;
using std::vector;
using std::unique_ptr;

using sigrok::ConfigKey;
using sigrok::Capability;

using Glib::VariantBase;
using Glib::Variant;

namespace sv {
namespace devices {

Device::~Device()
{
	//if (session_)
	//	session_->remove_datafeed_callbacks();

	// TODO:
	//remove_datafeed_callbacks();
}

shared_ptr<sigrok::Device> Device::sr_device() const
{
	return sr_device_;
}

template
uint64_t Device::read_config(const sigrok::ConfigKey*,
	const uint64_t);

template<typename T>
T Device::read_config(const ConfigKey *key, const T default_value)
{
	assert(key);

	if (!sr_device_)
		return default_value;

	if (!sr_device_->config_check(key, Capability::GET))
		return default_value;

	return VariantBase::cast_dynamic<Glib::Variant<guint64>>(
		sr_device_->config_get(ConfigKey::SAMPLERATE)).get();
}

void Device::feed_in_header()
{
}

void Device::feed_in_meta(shared_ptr<sigrok::Meta> meta)
{
	for (auto entry : meta->config()) {
		switch (entry.first->id()) {
		case SR_CONF_ENABLED:
			Q_EMIT enabled_changed(
				g_variant_get_boolean(entry.second.gobj()));
			break;
		case SR_CONF_VOLTAGE_TARGET:
			Q_EMIT voltage_target_changed(
				g_variant_get_double(entry.second.gobj()));
			break;
		case SR_CONF_CURRENT_LIMIT:
			Q_EMIT current_limit_changed(
				g_variant_get_double(entry.second.gobj()));
			break;
		case SR_CONF_OVER_VOLTAGE_PROTECTION_ACTIVE:
			Q_EMIT over_voltage_protection_active_changed(
				g_variant_get_boolean(entry.second.gobj()));
			break;
		case SR_CONF_OVER_CURRENT_PROTECTION_ACTIVE:
			Q_EMIT over_current_protection_active_changed(
				g_variant_get_boolean(entry.second.gobj()));
			break;
		case SR_CONF_UNDER_VOLTAGE_CONDITION_ACTIVE:
			Q_EMIT under_voltage_condition_active_changed(
				g_variant_get_boolean(entry.second.gobj()));
			break;
		case SR_CONF_OVER_TEMPERATURE_PROTECTION_ACTIVE:
			Q_EMIT over_temperature_protection_active_changed(
				g_variant_get_boolean(entry.second.gobj()));
			break;
		default:
			// Unknown metadata is not an error.
			break;
		}
	}

	//Q_EMIT signals_changed();
}

void Device::feed_in_trigger()
{
}

void Device::feed_in_frame_begin()
{
	frame_began_ = true;

	// TODO: use std::chrono / std::time and double
	qint64 time_span = (QDateTime::currentMSecsSinceEpoch() - time_start_);
	float dtime_span = time_span / (float)1000;
	// TODO: SignalBase / Analog class with long data?
	time_data_->push_sample(&dtime_span);
}

void Device::feed_in_frame_end()
{
	if (frame_began_)
		frame_began_ = false;
}

void Device::feed_in_analog(shared_ptr<sigrok::Analog> sr_analog)
{
	lock_guard<recursive_mutex> lock(data_mutex_);

	const vector<shared_ptr<sigrok::Channel>> sr_channels = sr_analog->channels();
	//const unsigned int channel_count = sr_channels.size();
	//const size_t sample_count = sr_analog->num_samples() / channel_count;

	unique_ptr<float> data(new float[sr_analog->num_samples()]);
	sr_analog->get_data_as_float(data.get());

	if (channel_data_.empty())
		update_signals();

	float *channel_data = data.get();
	qWarning("feed_in_analog(): data = %f", *channel_data);
	for (auto sr_channel : sr_channels) {
		qWarning() << "feed_in_analog(): Channel.Id = " << QString::fromStdString(sr_channel->name());

		channel_data_[sr_channel]->data()->push_sample(channel_data);
		channel_data++;

		// Timestamp for values not in a FRAME
		if (!frame_began_ &&
			!channel_data_[sr_channel]->internal_name().startsWith("V")) {
			// TODO: use std::chrono / std::time and double
			qint64 time_span = QDateTime::currentMSecsSinceEpoch() - time_start_;
			float dtime_span = time_span / (float)1000;
			// TODO: SignalBase / Analog class with long data?
			time_data_->push_sample(&dtime_span);
		}

		/*
		// Append the samples in the segment
		segment->append_interleaved_samples(channel_data++, sample_count,
			channel_count);

		Q_EMIT data_received(segment);
		*/
	}
}

void Device::data_feed_in(shared_ptr<sigrok::Device> sr_device,
	shared_ptr<sigrok::Packet> sr_packet)
{
	(void)sr_device;

	assert(sr_device);
	assert(sr_packet);

	if (sr_device != sr_device_)
		return;

	switch (sr_packet->type()->id()) {
	case SR_DF_HEADER:
		qWarning() << "data_feed_in(): SR_DF_HEADER";
		feed_in_header();
		break;

	case SR_DF_META:
		qWarning() << "data_feed_in(): SR_DF_META";
		feed_in_meta(
			dynamic_pointer_cast<sigrok::Meta>(sr_packet->payload()));
		break;

	case SR_DF_TRIGGER:
		qWarning() << "data_feed_in(): SR_DF_TRIGGER";
		feed_in_trigger();
		break;

	case SR_DF_LOGIC:
		qWarning() << "data_feed_in(): SR_DF_LOGIC";
		break;

	case SR_DF_ANALOG:
		try {
			feed_in_analog(
				dynamic_pointer_cast<sigrok::Analog>(sr_packet->payload()));
		} catch (bad_alloc) {
			out_of_memory_ = true;
			// TODO: sr_session->stop();
		}
		break;

	case SR_DF_FRAME_BEGIN:
		qWarning() << "data_feed_in(): SR_DF_FRAME_BEGIN";
		feed_in_frame_begin();
		break;

	case SR_DF_FRAME_END:
		qWarning() << "data_feed_in(): SR_DF_FRAME_END";
		feed_in_frame_end();
		break;

	case SR_DF_END:
		qWarning() << "data_feed_in(): SR_DF_END";
		// Strictly speaking, this is performed when a frame end marker was
		// received, so there's no point doing this again. However, not all
		// devices use frames, and for those devices, we need to do it here.
		{
			lock_guard<recursive_mutex> lock(data_mutex_);
		}
		break;

	default:
		break;
	}
}

void Device::update_signals()
{
	qWarning() << "update_signals() -1-";
	lock_guard<recursive_mutex> lock(data_mutex_);
	qWarning() << "update_signals() -2-";

	if (!sr_device_) {
		channel_data_.clear();

		/* TODO: Clear QwtPlot
		for (shared_ptr<views::ViewBase> view : views_) {
			view->clear_signals();
		}
		*/
		return;
	}

	// time signal
	time_start_ = QDateTime::currentMSecsSinceEpoch();
	time_data_ = make_shared<data::Analog>();

	// signals for sigrok channels
	auto sr_channels = sr_device_->channels();
	for (auto sr_channel : sr_device_->channels()) {

		switch(sr_channel->type()->id()) {
		case SR_CHANNEL_LOGIC:
			break;

		case SR_CHANNEL_ANALOG:
		{
			shared_ptr<data::SignalBase> signalbase =
				make_shared<data::SignalBase>(
					sr_channel, data::SignalBase::AnalogChannel);

			shared_ptr<data::Analog> data(new data::Analog());
			signalbase->set_data(data);

			channel_data_.insert(
				pair<shared_ptr<sigrok::Channel>,
				shared_ptr<data::SignalBase>>
					(sr_channel, signalbase));

			connect(this, SIGNAL(capture_state_changed(int)),
				signalbase.get(), SLOT(on_capture_state_changed(int)));

			break;
		}

		default:
			assert(false);
			break;
		}
	}

	//signals_changed();
}

} // namespace devices
} // namespace sv