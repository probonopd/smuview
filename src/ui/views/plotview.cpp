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

#include <cassert>

#include <QMainWindow>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

#include "plotview.hpp"
#include "src/session.hpp"
#include "src/channels/basechannel.hpp"
#include "src/data/analogsignal.hpp"
#include "src/ui/dialogs/plotconfigdialog.hpp"
#include "src/ui/dialogs/plotdiffmarkerdialog.hpp"
#include "src/ui/dialogs/selectsignaldialog.hpp"
#include "src/ui/widgets/plot/plot.hpp"
#include "src/ui/widgets/plot/basecurvedata.hpp"
#include "src/ui/widgets/plot/timecurvedata.hpp"
#include "src/ui/widgets/plot/xycurvedata.hpp"

using std::dynamic_pointer_cast;
using std::static_pointer_cast;

Q_DECLARE_METATYPE(sv::ui::widgets::plot::BaseCurveData *)

namespace sv {
namespace ui {
namespace views {

PlotView::PlotView(const Session &session,
		shared_ptr<channels::BaseChannel> channel,
		QWidget *parent) :
	BaseView(session, parent),
	initial_channel_(channel),
	action_add_marker_(new QAction(this)),
	action_add_diff_marker_(new QAction(this)),
	action_zoom_best_fit_(new QAction(this)),
	action_add_signal_(new QAction(this)),
	action_config_plot_(new QAction(this))
{
	assert(initial_channel_);

	shared_ptr<data::AnalogSignal> signal;
	if (initial_channel_->actual_signal())
		signal = static_pointer_cast<data::AnalogSignal>(
			initial_channel_->actual_signal());

	if (signal)
		curve_data_ = new widgets::plot::TimeCurveData(signal);
	else
		curve_data_ = nullptr;

	// Signal (aka Quantity + Flags + Unit) can change, e.g. DMM signals
	connect(initial_channel_.get(),
		SIGNAL(signal_added(shared_ptr<sv::data::BaseSignal>)),
		this, SLOT(on_signal_changed()));
	connect(initial_channel_.get(),
		SIGNAL(signal_changed(shared_ptr<sv::data::BaseSignal>)),
		this, SLOT(on_signal_changed()));

	setup_ui();
	setup_toolbar();
	connect_signals();
	init_values();

	plot_->start();
}

PlotView::PlotView(const Session& session,
		shared_ptr<sv::data::AnalogSignal> signal,
		QWidget* parent) :
	BaseView(session, parent),
	initial_channel_(nullptr),
	action_add_marker_(new QAction(this)),
	action_add_diff_marker_(new QAction(this)),
	action_zoom_best_fit_(new QAction(this)),
	action_add_signal_(new QAction(this)),
	action_config_plot_(new QAction(this))
{
	assert(signal);

	curve_data_ = new widgets::plot::TimeCurveData(signal);

	setup_ui();
	setup_toolbar();
	connect_signals();
	init_values();

	plot_->start();
}

PlotView::PlotView(const Session& session,
		shared_ptr<sv::data::AnalogSignal> x_signal,
		shared_ptr<sv::data::AnalogSignal> y_signal,
		QWidget* parent) :
	BaseView(session, parent),
	initial_channel_(nullptr),
	action_add_marker_(new QAction(this)),
	action_add_diff_marker_(new QAction(this)),
	action_zoom_best_fit_(new QAction(this)),
	action_add_signal_(new QAction(this)),
	action_config_plot_(new QAction(this))
{
	assert(x_signal);
	assert(y_signal);

	curve_data_ = new widgets::plot::XYCurveData(x_signal, y_signal);

	setup_ui();
	setup_toolbar();
	connect_signals();
	init_values();

	plot_->start();
}

QString PlotView::title() const
{
	QString title;

	if (initial_channel_)
		title = tr("Channel");
	else
		title = tr("Signal");

	if (curve_data_)
		title = title.append(" ").append(curve_data_->name());
	else if (initial_channel_)
		title = title.append(" ").append(initial_channel_->display_name());

	return title;
}

void PlotView::add_time_curve(shared_ptr<sv::data::AnalogSignal> signal)
{
	widgets::plot::TimeCurveData *curve_data =
		new widgets::plot::TimeCurveData(signal);
	plot_->add_curve(curve_data);
	update_add_marker_menu();
}

void PlotView::add_xy_curve(shared_ptr<sv::data::AnalogSignal> x_signal,
	shared_ptr<sv::data::AnalogSignal> y_signal)
{
	widgets::plot::XYCurveData *curve_data =
		new widgets::plot::XYCurveData(x_signal, y_signal);
	plot_->add_curve(curve_data);
	update_add_marker_menu();
}

void PlotView::setup_ui()
{
	QVBoxLayout *layout = new QVBoxLayout();

	plot_ = new widgets::plot::Plot();
	plot_->set_update_mode(widgets::plot::PlotUpdateMode::Additive);
	plot_->set_plot_interval(200); // 200ms
	if (curve_data_)
		plot_->add_curve(curve_data_);

	layout->addWidget(plot_);

	this->central_widget_->setLayout(layout);
}

void PlotView::setup_toolbar()
{
	add_marker_menu_ = new QMenu();
	update_add_marker_menu();

	add_marker_button_ = new QToolButton();
	add_marker_button_->setText(tr("Add marker"));
	add_marker_button_->setIcon(
		QIcon::fromTheme("snap-orthogonal",
		QIcon(":/icons/snap-orthogonal.png")));
	add_marker_button_->setMenu(add_marker_menu_);
	add_marker_button_->setPopupMode(QToolButton::MenuButtonPopup);

	action_add_diff_marker_->setText(tr("Add diff-marker"));
	action_add_diff_marker_->setIcon(
		QIcon::fromTheme("snap-guideline",
		QIcon(":/icons/snap-guideline.png")));
	connect(action_add_diff_marker_, SIGNAL(triggered(bool)),
		this, SLOT(on_action_add_diff_marker_triggered()));

	action_zoom_best_fit_->setText(tr("Best fit"));
	action_zoom_best_fit_->setIcon(
		QIcon::fromTheme("zoom-fit-best",
		QIcon(":/icons/zoom-fit-best.png")));
	connect(action_zoom_best_fit_, SIGNAL(triggered(bool)),
		this, SLOT(on_action_zoom_best_fit_triggered()));

	action_add_signal_->setText(tr("Add Signal"));
	action_add_signal_->setIcon(
		QIcon::fromTheme("office-chart-line",
		QIcon(":/icons/office-chart-line.png")));
	connect(action_add_signal_, SIGNAL(triggered(bool)),
		this, SLOT(on_action_add_signal_triggered()));

	action_config_plot_->setText(tr("Configure Plot"));
	action_config_plot_->setIcon(
		QIcon::fromTheme("configure",
		QIcon(":/icons/configure.png")));
	connect(action_config_plot_, SIGNAL(triggered(bool)),
		this, SLOT(on_action_config_plot_triggered()));

	toolbar_ = new QToolBar("Plot Toolbar");
	toolbar_->addWidget(add_marker_button_);
	toolbar_->addAction(action_add_diff_marker_);
	toolbar_->addSeparator();
	toolbar_->addAction(action_zoom_best_fit_);
	toolbar_->addSeparator();
	toolbar_->addAction(action_add_signal_);
	toolbar_->addSeparator();
	toolbar_->addAction(action_config_plot_);
	this->addToolBar(Qt::TopToolBarArea, toolbar_);
}

void PlotView::update_add_marker_menu()
{
	// First remove all existing actions
	for (QAction *action : add_marker_menu_->actions()) {
		disconnect(action, SIGNAL(triggered(bool)),
			this, SLOT(on_action_add_marker_triggered()));
		add_marker_menu_->removeAction(action);
		delete action;
	}

	// One add marker action for each curve
	for (widgets::plot::BaseCurveData *curve_data : plot_->curve_datas()) {
		QAction *action = new QAction(this);
		action->setText(curve_data->name());
		action->setData(QVariant::fromValue(curve_data));
		connect(action, SIGNAL(triggered(bool)),
			this, SLOT(on_action_add_marker_triggered()));
		add_marker_menu_->addAction(action);
	}
}

void PlotView::connect_signals()
{
}

void PlotView::init_values()
{
}

void PlotView::on_signal_changed()
{
	if (!initial_channel_)
		return;

	shared_ptr<sv::data::AnalogSignal> signal;
	if (initial_channel_->actual_signal())
		signal = dynamic_pointer_cast<sv::data::AnalogSignal>(
			initial_channel_->actual_signal());

	this->parentWidget()->setWindowTitle(this->title());

	if (signal) {
		curve_data_ = new widgets::plot::TimeCurveData(signal);
		plot_->add_curve(curve_data_);
		update_add_marker_menu();
	}
	else {
		curve_data_ = nullptr;
	}
}

void PlotView::on_action_add_marker_triggered()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		widgets::plot::BaseCurveData *curve_data =
			action->data().value<widgets::plot::BaseCurveData *>();
		plot_->add_marker(curve_data);
	}
}

void PlotView::on_action_add_diff_marker_triggered()
{
	ui::dialogs::PlotDiffMarkerDialog dlg(plot_);
	dlg.exec();
}

void PlotView::on_action_zoom_best_fit_triggered()
{
	plot_->set_all_axis_locked(false);
}

void PlotView::on_action_add_signal_triggered()
{
	ui::dialogs::SelectSignalDialog dlg(session(), nullptr);
	if (!dlg.exec())
		return;

	for (auto signal : dlg.signals())
		add_time_curve(dynamic_pointer_cast<sv::data::AnalogSignal>(signal));
}

void PlotView::on_action_config_plot_triggered()
{
	ui::dialogs::PlotConfigDialog dlg(plot_);
	dlg.exec();
}

} // namespace views
} // namespace ui
} // namespace sv
