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

#ifndef UI_DEVICES_CONFIGKEYCOMBOBOX_HPP
#define UI_DEVICES_CONFIGKEYCOMBOBOX_HPP

#include <memory>

#include <QComboBox>
#include <QWidget>

#include "src/devices/deviceutil.hpp"

using std::shared_ptr;

namespace sv {

namespace devices {
class Configurable;
}

namespace ui {
namespace devices {

class ConfigKeyComboBox : public QComboBox
{
	Q_OBJECT

public:
	ConfigKeyComboBox(shared_ptr<sv::devices::Configurable> configurable,
		QWidget *parent = nullptr);

	void set_configurable(shared_ptr<sv::devices::Configurable> configurable);
	void select_config_key(sv::devices::ConfigKey config_key);
	sv::devices::ConfigKey selected_config_key() const;

private:
	void setup_ui();
	void fill_config_keys();

	shared_ptr<sv::devices::Configurable> configurable_;

};

} // namespace devices
} // namespace ui
} // namespace sv

#endif // UI_DEVICES_CONFIGKEYCOMBOBOX_HPP
