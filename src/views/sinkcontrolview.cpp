/*
 * This file is part of the SmuView project.
 *
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

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "sinkcontrolview.hpp"
#include "src/session.hpp"
#include "src/devices/hardwaredevice.hpp"
#include "src/widgets/controlbutton.hpp"
#include "src/widgets/led.hpp"
#include "src/widgets/optionalvaluecontrol.hpp"
#include "src/widgets/valuecontrol.hpp"

namespace sv {
namespace views {

SinkControlView::SinkControlView(Session &session,
	shared_ptr<devices::HardwareDevice> device, QWidget *parent) :
		BaseView(session, parent),
	device_(device)
{
	setup_ui();
	connect_signals();
	init_values();
}

void SinkControlView::setup_ui()
{
	double min;
	double max;
	double step;

	QVBoxLayout *layout = new QVBoxLayout();

	// Enable button
	setEnableButton = new widgets::ControlButton(
		device_->is_enable_getable(), device_->is_enable_setable());
	layout->addWidget(setEnableButton);

	// Leds
	QGridLayout *ledLayout = new QGridLayout();
	// Regulation Leds
	ccLed = new widgets::Led("CC");
	ledLayout->addWidget(ccLed, 0, 0);
	cvLed = new widgets::Led("CV");
	ledLayout->addWidget(cvLed, 1, 0);
	// Protection Leds
	ovpLed = new widgets::Led("OVP");
	ledLayout->addWidget(ovpLed, 0, 1);
	ocpLed = new widgets::Led("OCP");
	ledLayout->addWidget(ocpLed, 0, 2);
	uvcLed = new widgets::Led("UVC");
	ledLayout->addWidget(uvcLed, 1, 1);
	otpLed = new widgets::Led("OTP");
	ledLayout->addWidget(otpLed, 1, 2);
	layout->addLayout(ledLayout, 0);

	// Curent limit
	device_->list_current_limit(min, max, step);
	setValueControl = new widgets::ValueControl(5, "A", min, max, step);
	layout->addWidget(setValueControl);

	// Under voltage threshold
	device_->list_under_voltage_threshold(min, max, step);
	setUnderVoltageThreshold = new widgets::OptionalValueControl(
		device_->is_under_voltage_threshold_getable(),
		device_->is_under_voltage_threshold_setable(),
		5, "V", min, max, step);
	layout->addWidget(setUnderVoltageThreshold);

	layout->addStretch(5);

	this->setLayout(layout);
}

void SinkControlView::connect_signals()
{
	// Control elements -> Device
	connect(setEnableButton, SIGNAL(state_changed(const bool)),
		this, SLOT(on_enabled_changed(const bool)));
	connect(setValueControl, SIGNAL(value_changed(const double)),
		this, SLOT(on_value_changed(const double)));
	connect(setUnderVoltageThreshold, SIGNAL(value_changed(const double)),
		this, SLOT(on_under_voltage_threshold_changed(const double)));

	// Device -> Control elements
	connect(device_.get(), SIGNAL(enabled_changed(const bool)),
		setEnableButton, SLOT(on_state_changed(const bool)));
	connect(device_.get(), SIGNAL(current_limit_changed(const double)),
		setValueControl, SLOT(change_value(const double)));
	connect(device_.get(),
		SIGNAL(under_voltage_condition_threshold_changed(const double)),
		setUnderVoltageThreshold, SLOT(change_value(const double)));

	connect(device_.get(),
		SIGNAL(over_voltage_protection_active_changed(const bool)),
		ovpLed, SLOT(on_state_changed(const bool)));
	connect(device_.get(),
		SIGNAL(over_current_protection_active_changed(const bool)),
		ocpLed, SLOT(on_state_changed(const bool)));
	connect(device_.get(),
		SIGNAL(under_voltage_condition_active_changed(const bool)),
		uvcLed, SLOT(on_state_changed(const bool)));
	connect(device_.get(),
		SIGNAL(over_temperature_protection_active_changed(const bool)),
		otpLed, SLOT(on_state_changed(const bool)));
}

void SinkControlView::init_values()
{
	// TODO: move to button (or whatever...)
	if (device_->is_enable_getable())
		setEnableButton->on_state_changed(device_->get_enabled());
	if (device_->is_current_limit_getable())
		setValueControl->on_value_changed(device_->get_current_limit());
	setUnderVoltageThreshold->on_state_changed(
		device_->get_under_voltage_enable());
	setUnderVoltageThreshold->on_value_changed(
		device_->get_under_voltage_threshold());

	if (device_->is_over_voltage_active_getable())
		ovpLed->on_state_changed(device_->get_over_voltage_active());
	if (device_->is_over_current_active_getable())
		ocpLed->on_state_changed(device_->get_over_current_active());
	if (device_->is_under_voltage_active_getable())
		uvcLed->on_state_changed(device_->get_under_voltage_active());
	if (device_->is_over_temperature_active_getable())
		otpLed->on_state_changed(device_->get_over_temperature_active());
}

void SinkControlView::on_enabled_changed(const bool enabled)
{
	device_->set_enable(enabled);
}

void SinkControlView::on_value_changed(const double value)
{
	device_->set_current_limit(value);
}

void SinkControlView::on_under_voltage_threshold_changed(const double value)
{
	device_->set_under_voltage_threshold(value);
}

} // namespace views
} // namespace sv
