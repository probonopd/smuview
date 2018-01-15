/*
 * This file is part of the SmuView project.
 *
 * Copyright (C) 2018 Frank Stettner <frank-stettner@gmx.net>
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

#ifndef CHANNELS_HARDWARECHANNEL_HPP
#define CHANNELS_HARDWARECHANNEL_HPP

#include <memory>
#include <QObject>

#include "src/channels/basechannel.hpp"

using std::shared_ptr;

namespace sigrok {
class Channel;
class Quantity;
class QuantityFlag;
class Unit;
}

namespace sv {

namespace data {
class BaseSignal;
}

namespace channels {

class HardwareChannel : public BaseChannel
{
	Q_OBJECT

public:
	HardwareChannel(
		shared_ptr<sigrok::Channel> sr_channel,
		const QString device_name,
		const QString channel_group_name,
		double channel_start_timestamp);

public:
	/**
	 * Returns enabled status of this channel.
	 */
	bool enabled() const;

	/**
	 * Sets the enabled status of this channel.
	 * @param value Boolean value to set.
	 */
	void set_enabled(bool value);

	/**
	 * Gets the index number of this sigrok channel
	 */
	unsigned int index() const;

	/**
	 * Sets the name of the signal.
	 */
	void set_name(QString name);

	/**
	 * Returns the underlying SR channel.
	 */
	shared_ptr<sigrok::Channel> sr_channel() const;

	/**
	 * Inits a signal
	 */
	shared_ptr<data::BaseSignal> init_signal(
		const sigrok::Quantity *sr_quantity,
		vector<const sigrok::QuantityFlag *> sr_quantity_flags,
		const sigrok::Unit *sr_unit);

protected:
	shared_ptr<sigrok::Channel> sr_channel_;

};

} // namespace channels
} // namespace sv

#endif // CHANNELS_HARDWARECHANNEL_HPP
