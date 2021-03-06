/*
 * This file is part of the SmuView project.
 *
 * Copyright (C) 2017-2018 Frank Stettner <frank-stettner@gmx.net>
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

#include <iostream>
#include <fstream>
#include <string>

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "savedialog.hpp"
#include "src/util.hpp"
#include "src/data/analogsignal.hpp"
#include "src/data/analogsignal.hpp"
#include "src/data/basesignal.hpp"
#include "src/devices/basedevice.hpp"
#include "src/devices/hardwaredevice.hpp"
#include "src/ui/devices/signaltree.hpp"

using std::ofstream;
using std::string;

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

namespace sv {
namespace ui {
namespace dialogs {

SaveDialog::SaveDialog(const Session &session,
		const vector<shared_ptr<sv::data::AnalogSignal>> selected_signals,
		QWidget *parent) :
	QDialog(parent),
	session_(session),
	selected_signals_(selected_signals)
{
	setup_ui();
}

void SaveDialog::setup_ui()
{
	QIcon main_icon;
	main_icon.addFile(QStringLiteral(":/icons/smuview.ico"),
		QSize(), QIcon::Normal, QIcon::Off);
	this->setWindowIcon(main_icon);
	this->setWindowTitle(tr("Save Signals"));
	this->setMinimumWidth(450);
	this->setMinimumHeight(400);

	QVBoxLayout *main_layout = new QVBoxLayout;

	signal_tree_ = new ui::devices::SignalTree(
		session_, true, true, true, nullptr);
	main_layout->addWidget(signal_tree_);

	QFormLayout *form_layout = new QFormLayout();

	timestamps_combined_ = new QCheckBox(tr("Combine all time stamps"));
	form_layout->addRow("", timestamps_combined_);

	time_absolut_ = new QCheckBox(tr("Absolut time"));
	form_layout->addRow("", time_absolut_);

	separator_edit_ = new QLineEdit();
	separator_edit_->setText(",");
	form_layout->addRow(tr("CSV separator"), separator_edit_);

	main_layout->addLayout(form_layout);

	button_box_ = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	main_layout->addWidget(button_box_);
	connect(button_box_, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box_, SIGNAL(rejected()), this, SLOT(reject()));

	this->setLayout(main_layout);
}

void SaveDialog::save(QString file_name)
{
	ofstream output_file;
	string str_file_name = file_name.toStdString();
	string str_separator = separator_edit_->text().toStdString();
	vector<size_t> sample_counts;

	output_file.open(str_file_name);

	auto signals = signal_tree_->selected_signals();
	bool relative_time = !time_absolut_->isChecked();
	QString sep = separator_edit_->text();
	size_t max_sample_count = 0;

	// Header
	QString start_sep("");
	QString device_header_line("");
	QString chg_name_header_line("");
	QString ch_name_header_line("");
	QString signal_name_header_line("");
	for (auto signal : signals) {
		size_t sample_count = signal->get_sample_count();
		if (sample_count > max_sample_count)
			max_sample_count = sample_count;
		sample_counts.push_back(sample_count);

		qWarning() << "SaveDialog::save(): signal.name() = " << signal->name();
		qWarning() << "SaveDialog::save(): signal.parent_channel().name() = " <<
			signal->parent_channel()->name();
		qWarning() << "SaveDialog::save(): signal.parent_channel().parent_device().name() = " <<
			signal->parent_channel()->parent_device()->name();

		device_header_line.append(start_sep).append(
			signal->parent_channel()->parent_device()->name()); // Time

		device_header_line.append(sep).append(
			signal->parent_channel()->parent_device()->name()); // Value
		chg_name_header_line.append(start_sep).append(
			signal->parent_channel()->channel_group_name()); // Time
		chg_name_header_line.append(sep).append(
			signal->parent_channel()->channel_group_name()); // Value
		ch_name_header_line.append(start_sep).append(
			signal->parent_channel()->name()); // Time
		ch_name_header_line.append(sep).append(
			signal->parent_channel()->name()); // Value
		signal_name_header_line.append(start_sep).
			append("Time ").append(signal->name()); // Time
		signal_name_header_line.append(sep).append(signal->name()); // Value

		start_sep = sep;
	}
	output_file << device_header_line.toStdString() << std::endl;
	output_file << chg_name_header_line.toStdString() << std::endl;
	output_file << ch_name_header_line.toStdString() << std::endl;
	output_file << signal_name_header_line.toStdString() << std::endl;

	// Data
	// TODO: we asume here, that the vector size is the same for all vectors....
	for (size_t i = 0; i < max_sample_count; i++) {
		start_sep = "";
		QString line("");
		for (size_t j = 0; j < signals.size(); j++) {
			QString time("");
			QString value("");

			size_t sample_count = sample_counts[j];
			if (i < sample_count-1) {
				// More samples for this signal
				auto a_signal = static_pointer_cast<data::AnalogSignal>(
					signals.at(j));
				data::sample_t sample = a_signal->get_sample(i, relative_time);
				value = QString("%1").arg(sample.second);
				if (relative_time)
					time = QString("%1").arg(sample.first);
				else
					time = util::format_time_date(sample.first);
			}

			line.append(QString("%1%2%3%4").arg(start_sep).arg(time).
				arg(sep).arg(value));
			start_sep = sep;
		}
		output_file << line.toStdString() << std::endl;
	}

	output_file.close();
}

void SaveDialog::save_combined(QString file_name)
{
	ofstream output_file;
	string str_file_name = file_name.toStdString();
	string str_separator = separator_edit_->text().toStdString();
	vector<size_t> sample_counts;
	vector<size_t> sample_pos;

	output_file.open(str_file_name);

	auto signals = signal_tree_->selected_signals();
	bool relative_time = !time_absolut_->isChecked();
	QString sep = separator_edit_->text();

	// Header
	QString device_header_line("Time"); // Time
	QString chg_name_header_line("Time"); // Time
	QString ch_name_header_line("Time"); // Time
	QString signal_name_header_line("Time"); // Time
	for (auto signal : signals) {
		auto a_signal = static_pointer_cast<data::AnalogSignal>(signal);
		sample_counts.push_back(a_signal->get_sample_count());
		sample_pos.push_back(0);

		device_header_line.append(sep).append(
			signal->parent_channel()->parent_device()->name()); // Value
		chg_name_header_line.append(sep).append(
			signal->parent_channel()->channel_group_name()); // Value
		ch_name_header_line.append(sep).append(
			signal->parent_channel()->name()); // Value
		signal_name_header_line.append(sep).append(signal->name()); // Value
	}
	output_file << device_header_line.toStdString() << std::endl;
	output_file << chg_name_header_line.toStdString() << std::endl;
	output_file << ch_name_header_line.toStdString() << std::endl;
	output_file << signal_name_header_line.toStdString() << std::endl;

	// Data
	while (true) {
		double next_timestamp = -1;
		for (size_t i=0; i<signals.size(); i++) {
			if (sample_pos[i] >= sample_counts[i]-1)
				continue;

			auto a_signal = static_pointer_cast<data::AnalogSignal>(signals[i]);
			double timestamp =
				a_signal->get_sample(sample_pos[i], relative_time).first;
			if (next_timestamp < 0 || timestamp < next_timestamp)
				next_timestamp = timestamp;
		}

		if (next_timestamp < 0)
			break;

		// Timestamp
		QString line;
		if (relative_time)
			line = QString("%1").arg(next_timestamp, 0, 'f', 4);
		else
			line = util::format_time_date(next_timestamp);

		// Values
		for (size_t i=0; i<signals.size(); i++) {
			line.append(sep);

			auto a_signal = static_pointer_cast<data::AnalogSignal>(signals[i]);
			data::sample_t sample =
				a_signal->get_sample(sample_pos[i], relative_time);
			double timestamp = sample.first;
			if (timestamp == next_timestamp) {
				line.append(QString("%1").arg(sample.second, 0, 'g', -1));
				++sample_pos[i];
			}
		}
		output_file << line.toStdString() << std::endl;
	}

	output_file.close();
}

void SaveDialog::accept()
{
	// Get file name
	QString file_name = QFileDialog::getSaveFileName(this,
		tr("Save CSV-File"), QDir::homePath(), tr("CSV Files (*.csv)"));

	if (file_name.length() > 0) {
		if (timestamps_combined_->isChecked())
			save_combined(file_name);
		else
			save(file_name);

		QDialog::accept();
	}
}

} // namespace dialogs
} // namespace ui
} // namespace sv
