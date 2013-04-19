/*
 * Compiz Autopilot GTest Acceptance Tests
 *
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authored By:
 * Sam Spilsbury <smspillaz@gmail.com>
 */

#include <stdexcept>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <gtest/gtest.h>

using ::testing::ValuesIn;
using ::testing::WithParamInterface;

namespace
{
    int launchBinary (const std::string         &executable,
		      const char                **argv,
		      int                       &stderrWriteEnd,
		      int                       &stdoutWriteEnd)
    {
	/* Save stderr */
	int stderr = dup (STDERR_FILENO);

	if (stderr == -1)
	    throw std::runtime_error (strerror (errno));

	/* Save stdout */
	int stdout = dup (STDOUT_FILENO);

	if (stdout == -1)
	    throw std::runtime_error (strerror (errno));

	/* Drop reference */
	if (close (STDERR_FILENO) == -1)
	    throw std::runtime_error (strerror (errno));

	/* Drop reference */
	if (close (STDOUT_FILENO) == -1)
	    throw std::runtime_error (strerror (errno));

	/* Make the pipe write end our stderr */
	if (dup2 (stderrWriteEnd, STDERR_FILENO) == -1)
	    throw std::runtime_error (strerror (errno));

	/* Make the pipe write end our stdout */
	if (dup2 (stdoutWriteEnd, STDOUT_FILENO) == -1)
	    throw std::runtime_error (strerror (errno));

	pid_t child = fork ();

	/* Child process */
	if (child == 0)
	{
	    if (execvpe (executable.c_str (),
			 const_cast <char * const *> (argv),
			 environ) == -1)
	    {
		std::cerr << "execvpe failed with error "
			  << errno
			  << std::endl
			  << " - binary "
			  << executable
			  << std::endl;
			  abort ();
	    }
	}
	/* Parent process - error */
	else if (child == -1)
	{
	    throw std::runtime_error (strerror (errno));
	}
	else
	{
	    /* Redirect old stderr back to stderr */
	    if (dup2 (stderr, STDERR_FILENO) == -1)
		throw std::runtime_error (strerror (errno));

	    /* Redirect old stderr back to stderr */
	    if (dup2 (stdout, STDOUT_FILENO) == -1)
		throw std::runtime_error (strerror (errno));

	    /* Close the write end of the pipe - its being
	     * used by the child */
	    if (close (stderrWriteEnd) == -1)
		throw std::runtime_error (strerror (errno));

	    /* Close the write end of the pipe - its being
	     * used by the child */
	    if (close (stdoutWriteEnd) == -1)
		throw std::runtime_error (strerror (errno));

	    stderrWriteEnd = 0;
	    stdoutWriteEnd = 0;

	    int status = 0;

	    do
	    {
		pid_t waitChild = waitpid (child, &status, 0);
		if (waitChild == child)
		{
                    if (WIFSIGNALED (status))
                    {
                        std::stringstream ss;
                        ss << "child killed by signal "
                           << WTERMSIG (status);
                        throw std::runtime_error (ss.str ());
                    }
		}
		else
		    throw std::runtime_error (strerror (errno));
	    } while (!WIFEXITED (status) && !WIFSIGNALED (status));

	    return WEXITSTATUS (status);
	}

	throw std::logic_error ("unreachable section");
    }

    const char *autopilot = "/usr/bin/autopilot";
    const char *runOpt = "run";
    const char *dashV = "-v";
}

class CompizAutopilotAcceptanceTest :
    public ::testing::Test,
    public ::testing::WithParamInterface <const char *>
{
    public:

	CompizAutopilotAcceptanceTest ();
	~CompizAutopilotAcceptanceTest ();
	const char ** GetAutopilotArgv ();
	void PrintChildStderr ();
	void PrintChildStdout ();

    protected:

	std::vector <const char *> autopilotArgv;
	int                        childStdoutPipe[2];
	int                        childStderrPipe[2];
};

CompizAutopilotAcceptanceTest::CompizAutopilotAcceptanceTest ()
{
    if (pipe2 (childStderrPipe, O_CLOEXEC) == -1)
    {
	throw std::runtime_error (strerror (errno));
    }

    if (pipe2 (childStdoutPipe, O_CLOEXEC) == -1)
    {
	throw std::runtime_error (strerror (errno));
    }

    autopilotArgv.push_back (autopilot);
    autopilotArgv.push_back (runOpt);
    autopilotArgv.push_back (dashV);
    autopilotArgv.push_back (GetParam ());
    autopilotArgv.push_back (NULL);
}

CompizAutopilotAcceptanceTest::~CompizAutopilotAcceptanceTest ()
{
    if (childStderrPipe[0] &&
	close (childStderrPipe[0]) == -1)
	std::cerr << "childStderrPipe[0] " << strerror (errno) << std::endl;

    if (childStderrPipe[1] &&
	close (childStderrPipe[1]) == -1)
	std::cerr << "childStderrPipe[1] " << strerror (errno) << std::endl;

    if (childStdoutPipe[0] &&
	close (childStdoutPipe[0]) == -1)
	std::cerr << "childStdoutPipe[0] " << strerror (errno) << std::endl;

    if (childStdoutPipe[1] &&
	close (childStdoutPipe[1]) == -1)
	std::cerr << "childStdoutPipe[1] " << strerror (errno) << std::endl;
}

const char **
CompizAutopilotAcceptanceTest::GetAutopilotArgv ()
{
    return &autopilotArgv[0];
}

namespace
{
    std::string FdToString (int fd)
    {
	std::string output;

	int bufferSize = 4096;
	char buffer[bufferSize];

	ssize_t count = 0;

	do
	{
	    count = read (fd, (void *) buffer, bufferSize - 1);
	    if (count == -1)
		throw std::runtime_error (strerror (errno));

	    buffer[count] = '\0';

	    output += buffer;
	} while (count != 0);

	return output;
    }
}

void
CompizAutopilotAcceptanceTest::PrintChildStderr ()
{
    std::string output = FdToString (childStderrPipe[0]);

    std::cout << "[== TEST ERRORS ==]" << std::endl
	      << output
	      << std::endl;
}

void
CompizAutopilotAcceptanceTest::PrintChildStdout ()
{
    std::string output = FdToString (childStdoutPipe[0]);

    std::cout << "[== TEST MESSAGES ==]" << std::endl
	      << output
	      << std::endl;
}

TEST_P (CompizAutopilotAcceptanceTest, AutopilotTest)
{
    std::string scopedTraceMsg ("Running Autopilot Test");
    scopedTraceMsg += GetParam ();

    int status = launchBinary (std::string (autopilot),
			       GetAutopilotArgv (),
			       childStderrPipe[1],
			       childStdoutPipe[1]);

    EXPECT_EQ (status, 0) << "expected exit status of 0";

    if (status)
    {
	PrintChildStdout ();
	PrintChildStderr ();
    }
    else
	std::cout << "[AUTOPILOT ] Pass test " << GetParam () << std::endl;
}

namespace
{
const char *AutopilotTests[] =
{
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_clicking_icon_twice_initiates_spread",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_expo_launcher_icon_initiates_expo",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_expo_launcher_icon_terminates_expo",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_launcher_activate_last_focused_window",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_unminimize_initially_minimized_windows",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_unminimize_minimized_immediately_after_show_windows",
    "unity.tests.launcher.test_icon_behavior.LauncherIconsTests.test_while_in_scale_mode_the_dash_will_still_open",
    "unity.tests.test_dash.DashRevealWithSpreadTests.test_command_lens_opens_when_in_spread",
    "unity.tests.test_dash.DashRevealWithSpreadTests.test_dash_closes_on_spread",
    "unity.tests.test_dash.DashRevealWithSpreadTests.test_dash_opens_when_in_spread",
    "unity.tests.test_dash.DashRevealWithSpreadTests.test_lens_opens_when_in_spread",
    "unity.tests.test_hud.HudBehaviorTests.test_alt_arrow_keys_not_eaten",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_panel_title_updates_moving_window",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_close_inactive_when_clicked_in_another_monitor",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_dont_show_for_maximized_window_on_mouse_in",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_dont_show_in_other_monitors_when_dash_is_open",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_dont_show_in_other_monitors_when_hud_is_open",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_minimize_inactive_when_clicked_in_another_monitor",
    "unity.tests.test_panel.PanelCrossMonitorsTests.test_window_buttons_unmaximize_inactive_when_clicked_in_another_monitor",
    "unity.tests.test_panel.PanelGrabAreaTests.test_focus_the_maximized_window_works",
    "unity.tests.test_panel.PanelGrabAreaTests.test_lower_the_maximized_window_works",
    "unity.tests.test_panel.PanelGrabAreaTests.test_panels_dont_steal_keynav_foucs_from_hud",
    "unity.tests.test_panel.PanelGrabAreaTests.test_unmaximize_from_grab_area_works",
    "unity.tests.test_panel.PanelHoverTests.test_menus_show_for_maximized_window_on_mouse_in_btn_area",
    "unity.tests.test_panel.PanelHoverTests.test_menus_show_for_maximized_window_on_mouse_in_grab_area",
    "unity.tests.test_panel.PanelHoverTests.test_menus_show_for_maximized_window_on_mouse_in_menu_area",
    "unity.tests.test_panel.PanelHoverTests.test_only_menus_show_for_restored_window_on_mouse_in_grab_area",
    "unity.tests.test_panel.PanelHoverTests.test_only_menus_show_for_restored_window_on_mouse_in_menu_area",
    "unity.tests.test_panel.PanelHoverTests.test_only_menus_show_for_restored_window_on_mouse_in_window_btn_area",
    "unity.tests.test_panel.PanelMenuTests.test_menus_dont_show_for_maximized_window_on_mouse_out",
    "unity.tests.test_panel.PanelMenuTests.test_menus_dont_show_for_restored_window_on_mouse_out",
    "unity.tests.test_panel.PanelMenuTests.test_menus_dont_show_if_a_new_application_window_is_opened",
    "unity.tests.test_panel.PanelMenuTests.test_menus_show_for_maximized_window_on_mouse_in",
    "unity.tests.test_panel.PanelMenuTests.test_menus_show_for_restored_window_on_mouse_in",
    "unity.tests.test_panel.PanelMenuTests.test_menus_shows_when_new_application_is_opened",
    "unity.tests.test_panel.PanelTitleTests.test_panel_shows_app_title_with_maximised_app",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_doesnt_change_with_switcher",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_on_empty_desktop",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_updates_on_maximized_window_title_changes",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_updates_when_switching_to_maximized_app",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_with_maximized_application",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_with_maximized_window_restored_child",
    "unity.tests.test_panel.PanelTitleTests.test_panel_title_with_restored_application",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_double_click_unmaximize_window",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_minimize_button_disabled_for_non_minimizable_windows",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_dont_show_for_maximized_window_on_mouse_out",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_dont_show_for_restored_window",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_dont_show_for_restored_window_with_mouse_in_panel",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_dont_show_on_empty_desktop",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_minimize_button_works_for_window",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_show_for_maximized_window_on_mouse_in",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_unmaximize_button_works_for_window",
    "unity.tests.test_panel.PanelWindowButtonsTests.test_window_buttons_unmaximize_follows_fitts_law",
    "unity.tests.test_showdesktop.ShowDesktopTests.test_showdesktop_hides_apps",
    "unity.tests.test_showdesktop.ShowDesktopTests.test_showdesktop_switcher",
    "unity.tests.test_showdesktop.ShowDesktopTests.test_showdesktop_unhides_apps",
    "unity.tests.test_showdesktop.ShowDesktopTests.test_unhide_single_app",
    "unity.tests.test_spread.SpreadTests.test_scale_application_windows",
    "unity.tests.test_spread.SpreadTests.test_scaled_window_closes_on_close_button_click",
    "unity.tests.test_spread.SpreadTests.test_scaled_window_closes_on_middle_click",
    "unity.tests.test_spread.SpreadTests.test_scaled_window_is_focused_on_click",
    "unity.tests.test_switcher.SwitcherDetailsModeTests.test_detail_mode_selects_last_active_window",
    "unity.tests.test_switcher.SwitcherDetailsModeTests.test_detail_mode_selects_third_window",
    "unity.tests.test_switcher.SwitcherDetailsTests.test_no_details_for_apps_on_different_workspace",
    "unity.tests.test_switcher.SwitcherTests.test_application_window_is_fake_decorated",
    "unity.tests.test_switcher.SwitcherTests.test_application_window_is_fake_decorated_in_detail_mode",
    "unity.tests.test_switcher.SwitcherWindowsManagementTests.test_switcher_raises_only_last_focused_window",
    "unity.tests.test_switcher.SwitcherWindowsManagementTests.test_switcher_rises_next_window_of_same_application",
    "unity.tests.test_switcher.SwitcherWindowsManagementTests.test_switcher_rises_other_application",
    "unity.tests.test_switcher.SwitcherWorkspaceTests.test_switcher_all_mode_shows_all_apps",
    "unity.tests.test_switcher.SwitcherWorkspaceTests.test_switcher_can_switch_to_minimised_window",
    "unity.tests.test_switcher.SwitcherWorkspaceTests.test_switcher_is_disabled_when_wall_plugin_active",
    "unity.tests.test_switcher.SwitcherWorkspaceTests.test_switcher_shows_current_workspace_only"
};
}


INSTANTIATE_TEST_CASE_P (UnityIntegrationAutopilotTests, CompizAutopilotAcceptanceTest,
			 ValuesIn (AutopilotTests));
