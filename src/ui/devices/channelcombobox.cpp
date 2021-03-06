/*
 * This file is part of the SmuView project.
 *
 * Copyright (C) 2018 Frank Stettner <frank-stettner@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <map>
#include <memory>
#include <vector>

#include <QDebug>
#include <QVariant>

#include "channelcombobox.hpp"
#include "src/session.hpp"
#include "src/channels/basechannel.hpp"
#include "src/devices/basedevice.hpp"

using std::shared_ptr;
using std::vector;

Q_DECLARE_METATYPE(shared_ptr<sv::channels::BaseChannel>)

namespace sv {
namespace ui {
namespace devices {

ChannelComboBox::ChannelComboBox(
		const Session &session, shared_ptr<sv::devices::BaseDevice> device,
		QString channel_group, QWidget *parent) :
	QComboBox(parent),
	session_(session),
	device_(device),
	channel_group_(channel_group)
{
	setup_ui();
}

void ChannelComboBox::select_channel(
	shared_ptr<sv::channels::BaseChannel> channel)
{
	for (int i = 0; i < this->count(); ++i) {
		QVariant data = this->itemData(i, Qt::UserRole);
		auto item_channel = data.value<shared_ptr<sv::channels::BaseChannel>>();
		if (item_channel == channel) {
			this->setCurrentIndex(i);
			break;
		}
	}
}

shared_ptr<sv::channels::BaseChannel> ChannelComboBox::selected_channel() const
{
	QVariant data = this->currentData();
	return data.value<shared_ptr<sv::channels::BaseChannel>>();
}

void ChannelComboBox::setup_ui()
{
	assert(device_);

	vector<shared_ptr<sv::channels::BaseChannel>> channels;
	if (channel_group_ == nullptr || channel_group_.isEmpty()) {
		for (auto ch_name_pair : device_->channel_name_map()) {
			this->addItem(
				ch_name_pair.first, QVariant::fromValue(ch_name_pair.second));
		}
	}
	else {
		if (!device_->channel_group_name_map().count(channel_group_))
			return;

		auto ch_list = device_->channel_group_name_map()[channel_group_];
		for (auto ch : ch_list) {
			this->addItem(ch->name(), QVariant::fromValue(ch));
		}
	}
}

void ChannelComboBox::change_device_channel_group(
	shared_ptr<sv::devices::BaseDevice> device, QString channel_group)
{
	device_ = device;
	channel_group_ = channel_group;
	for (int i = this->count(); i >= 0; --i)
		this->removeItem(i);
	this->setup_ui();
}

} // namespace devices
} // namespace ui
} // namespace sv
