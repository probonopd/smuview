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

#include <memory>

#include <QDebug>
#include <QVariant>

#include "devicecombobox.hpp"
#include "src/session.hpp"
#include "src/devices/device.hpp"

using std::shared_ptr;

Q_DECLARE_METATYPE(shared_ptr<sv::devices::Device>)

namespace sv {
namespace widgets {

DeviceComboBox::DeviceComboBox(const Session &session, QWidget *parent) :
	QComboBox(parent),
	session_(session)
{
	setup_ui();
}

void DeviceComboBox::select_device(shared_ptr<devices::Device> device)
{
	for (int i = 0; i < this->count(); ++i) {
		QVariant data = this->itemData(i, Qt::UserRole);
		auto item_device = data.value<shared_ptr<devices::Device>>();
		if (item_device == device) {
			this->setCurrentIndex(i);
			break;
		}
	}
}

const shared_ptr<devices::Device> DeviceComboBox::selected_device()
{
	QVariant data = this->currentData();
	return data.value<shared_ptr<devices::Device>>();
}

void DeviceComboBox::setup_ui()
{
	for (auto device : session_.devices()) {
		this->addItem(
			device->full_name(), QVariant::fromValue(device));
	}
}

} // namespace widgets
} // namespace sv
