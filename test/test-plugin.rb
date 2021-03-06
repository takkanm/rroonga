# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

class PluginTest < Test::Unit::TestCase
  include GroongaTestUtils

  setup :setup_sandbox
  setup :setup_database
  teardown :teardown_sandbox

  def test_register
    context = Groonga::Context.default
    assert_nil(context["suggest"])
    context.register_plugin("suggest/suggest")
    assert_not_nil(context["suggest"])
  end

  def test_system_plugins_dir
    suggest_plugin_path = "#{Groonga::Plugin.system_plugins_dir}/"
    suggest_plugin_path << "suggest/suggest#{Groonga::Plugin.suffix}"
    assert_send([File, :exist?, suggest_plugin_path])
  end
end
