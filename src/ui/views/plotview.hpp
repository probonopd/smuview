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

#ifndef UI_VIEWS_PLOTVIEW_HPP
#define UI_VIEWS_PLOTVIEW_HPP

#include <memory>

#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

#include "src/ui/views/baseview.hpp"

using std::shared_ptr;

namespace sv {

class Session;

namespace channels {
class BaseChannel;
}
namespace data {
class AnalogSignal;
}

namespace ui {

namespace widgets {
namespace plot {
class BaseCurveData;
class Plot;
}
}

namespace views {

class PlotView : public BaseView
{
	Q_OBJECT

public:
	PlotView(const Session& session,
		shared_ptr<channels::BaseChannel> channel,
		QWidget* parent = nullptr);
	PlotView(const Session& session,
		shared_ptr<sv::data::AnalogSignal> signal,
		QWidget* parent = nullptr);
	PlotView(const Session& session,
		shared_ptr<sv::data::AnalogSignal> x_signal,
		shared_ptr<sv::data::AnalogSignal> y_signal,
		QWidget* parent = nullptr);

	QString title() const;
	void add_time_curve(shared_ptr<sv::data::AnalogSignal> signal);
	void add_xy_curve(shared_ptr<sv::data::AnalogSignal> x_signal,
		shared_ptr<sv::data::AnalogSignal> y_signal);

private:
	void setup_ui();
	void setup_toolbar();
	void update_add_marker_menu();
	void connect_signals();
	void init_values();

	shared_ptr<channels::BaseChannel> initial_channel_;
	widgets::plot::BaseCurveData *curve_data_; // TODO: to vector

	QMenu *add_marker_menu_;
	QToolButton *add_marker_button_;
	QAction *const action_add_marker_;
	QAction *const action_add_diff_marker_;
	QAction *const action_zoom_best_fit_;
	QAction *const action_add_signal_;
	QAction *const action_config_plot_;
	QToolBar *toolbar_;
	widgets::plot::Plot *plot_;

private Q_SLOTS:
	void on_signal_changed();
	void on_action_add_marker_triggered();
	void on_action_add_diff_marker_triggered();
	void on_action_zoom_best_fit_triggered();
	void on_action_add_signal_triggered();
	void on_action_config_plot_triggered();

};

} // namespace views
} // namespace ui
} // namespace sv

#endif // UI_VIEWS_PLOTVIEW_HPP
