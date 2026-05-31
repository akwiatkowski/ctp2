project('ctp2', 'c', 'cpp', version : '1.2', subproject_dir : 'libs',
        default_options : ['cpp_std=c++20'])

conf = configuration_data()
conf.set_quoted('VERSION', meson.version())

# Capture the current git short SHA at configure time so saves can embed
# a build identifier ("ctp2_build" key in JSON saves).  Falls back to
# "unknown" outside a git checkout.  Run with check:false so a missing
# git binary or non-checkout doesn't fail configure.
git_sha_result = run_command('git', 'rev-parse', '--short=12', 'HEAD',
                             check: false)
if git_sha_result.returncode() == 0
    conf.set_quoted('CTP2_BUILD_SHA', git_sha_result.stdout().strip())
else
    conf.set_quoted('CTP2_BUILD_SHA', 'unknown')
endif

conf.set('SHOW_ASSERTS', get_option('buildtype').startswith('debug'))
conf.set('USE_LOGGING', get_option('buildtype').startswith('debug'))
conf.set('_DEBUG', get_option('buildtype').startswith('debug'))

if get_option('buildtype').startswith('debug')
	add_global_arguments('-D_DEBUG', language : ['c', 'cpp'])
endif

# first look into our own bin folder, then search the PATH
flex = find_program('../bin/flex', 'flex')
byacc = find_program('../bin/byacc', 'byacc')

lgen = generator(flex,
	output : 'lex.@BASENAME@.c',
	arguments : ['-i', '-8', '@EXTRA_ARGS@', '-o', '@OUTPUT@', '@INPUT@'])

pgen = generator(byacc,
	output : ['@BASENAME@.tab.c', '@BASENAME@.tab.h'],
	arguments : ['-d', '@EXTRA_ARGS@', '-o', '@OUTPUT0@', '@INPUT@'])

unzip = find_program('unzip')

compiler = meson.get_compiler('cpp')
win32 = compiler.has_header('windows.h')

incdir = []
main_dependencies = []
use_ld = []
host_system = host_machine.system()

if not win32
	# use gold-linker by default if available (Linux only)
	if host_system == 'linux'
		gold = find_program('gold', required: false)
		if gold.found()
			add_global_arguments('-fuse-ld=gold', language : ['c', 'cpp'])
		endif
	endif

	sdl = [dependency('sdl2'), dependency('SDL2_mixer')]
	conf.set('USE_SDL', 1)

	tiff = dependency('libtiff-4')
	zlib = dependency('zlib')

	# Modern structured logging.  Replaces ad-hoc fprintf(stderr, "[TAG]...")
	# calls that already exist for [HEADLESS] / [CIVAPP] traces.  Provides
	# compile-time-filterable levels, named loggers per subsystem, sinks
	# (stderr / file / rotating), and fmt-style format strings.  Headers
	# only — no runtime link cost beyond fmt.
	spdlog = dependency('spdlog')

	# GTK is only used for message dialogs on Linux
	gtk = dependency('gtk+-3.0', required: false)
	if gtk.found()
		conf.set('USE_GTK', 1)
	endif

	x11 = dependency('x11', required: false)
	if x11.found()
		conf.set('HAVE_X11', 1, description : 'System with X11 present')
	endif
	main_dependencies += [tiff, zlib, sdl, x11, spdlog]

	# nowin32 stubs: depend on gtk only if available
	main_dependencies += declare_dependency( sources : 'os/nowin32/nowin32.cpp', dependencies : gtk )
	incdir += include_directories('os/nowin32')
	incdir += include_directories('user-robotcom')

	conf.set_quoted('FILE_SEP', '/')
	conf.set('FILE_SEPC', '\'/\'')
	conf.set_quoted('PATH_SEP', ':')
	conf.set('PATH_SEPC', '\':\'')

	conf.set('USE_COM_REPLACEMENT', 1, description : 'Do not use COM')

	# libc++ hardening: use DEBUG mode for debug builds (assertions with messages),
	# FAST mode for release (minimal overhead, aborts on violation)
	if host_system == 'darwin'
		if get_option('buildtype').startswith('debug')
			add_global_arguments('-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG', language : ['cpp'])
		else
			add_global_arguments('-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST', language : ['cpp'])
		endif
	endif
endif

conf.set('CTP2_ENABLE_SLICDEBUG', 0, description : 'Enable slic debugging code')
conf.set('_BFR_', 1, description : 'Build final release')

# Compiler hardening flags for non-Windows builds
hardening_level = get_option('hardening_level')
if not win32
	if hardening_level != 'none'
		# Layer 1: Stack protection — canaries on functions with arrays/alloca
		add_global_arguments('-fstack-protector-strong', language : ['c', 'cpp'])

		# Layer 1: libc fortification — compile-time + runtime bounds checking
		add_global_arguments('-D_FORTIFY_SOURCE=3', language : ['c', 'cpp'])
	endif

	if hardening_level == 'strong' or hardening_level == 'maximum'
		# Use add_project_arguments (not global) so subprojects like anet aren't affected.
		# Catch format string mismatches at compile time (addresses ~65 FORMAT_STRING_BUGs).
		# Use -Wformat=2 (warning) rather than -Werror=format for now — the codebase
		# has many format mismatches that need systematic cleanup. See hardening plan.
		add_project_arguments('-Wformat=2', language : ['c', 'cpp'])
		add_project_arguments('-Werror=format-security', language : ['c', 'cpp'])

		# Catch functions missing return values on some paths
		add_project_arguments('-Werror=return-type', language : ['c', 'cpp'])

		# Catch use of uninitialized variables
		add_project_arguments('-Werror=uninitialized', language : ['c', 'cpp'])

		# Catch implicit function declarations (often lead to wrong calling conventions)
		add_project_arguments('-Werror=implicit-function-declaration', language : ['c'])

		# String / buffer safety warnings — catch the bug classes introduced by the
		# sprintf→snprintf and strcat→strncat conversion batches. The key one is
		# -Wsizeof-pointer-memaccess, which detects sizeof(ptr) used as a buffer-size
		# argument (the trap that bit batch 59 in CivPaths::MakeSavePath).
		# Some flags are GCC-only; use get_supported_arguments to filter portably.
		string_safety_warnings = compiler.get_supported_arguments([
			'-Wsizeof-pointer-memaccess',
			'-Wstringop-overflow=4',
			'-Wstringop-truncation',
			'-Wformat-truncation=2',
			'-Wmemset-transposed-args',
		])
		add_project_arguments(string_safety_warnings, language : ['c', 'cpp'])
	endif

	if hardening_level == 'maximum'
		# Trap on signed integer overflow (UB that ASan doesn't always catch)
		add_global_arguments('-ftrapv', language : ['c', 'cpp'])

		# Extra format warnings before they become errors
		add_global_arguments('-Wformat=2', language : ['c', 'cpp'])

		# Enable _GLIBCXX_ASSERTIONS on Linux (libstdc++ bounds checking)
		if host_system == 'linux'
			add_global_arguments('-D_GLIBCXX_ASSERTIONS', language : ['cpp'])
		endif
	endif

	# ftrapv is always enabled in debug builds regardless of hardening level
	if get_option('buildtype').startswith('debug') and hardening_level != 'maximum'
		add_global_arguments('-ftrapv', language : ['c', 'cpp'])
	endif
endif

if compiler.has_header('linux/kernel.h')
	conf.set('LINUX', 1)
endif

configure_file(output : 'config.h', configuration : conf)
add_global_arguments('-DHAVE_CONFIG_H', language : ['c', 'cpp'])
add_global_arguments('-DHAVE_UNISTD_H', language : ['c', 'cpp'])
add_global_arguments('-DHAVE_STDINT_H', language : ['c', 'cpp'])
add_global_arguments('-DHAVE_STRING_H', language : ['c', 'cpp'])
add_global_arguments('-Wno-non-pod-varargs', language : ['cpp'])
add_global_arguments('-Wno-implicit-function-declaration', language : ['c'])

anet = subproject('anet').get_variable('libanet')

freetype = subproject('freetype-1.3.1').get_variable('libfreetype')

incdir += include_directories(
	'.',
	'os/include',
	'ctp',
	'ctp/ctp2_utils',
	'ctp/ctp2_rsrc',
	'ctp/debugtools',
	'ctp/fingerprint',
	'robot/utility',
	'robot/pathing',
	'robot/aibackdoor',
	'GameWatch/gamewatch',
	'gfx/gfx_utils',
	'gfx/spritesys',
	'gfx/tilesys',
	'gfx/layers',
	'gs/database',
	'gs/events',
	'gs/fileio',
	'gs/gameobj',
	'gs/newdb',
	'gs/slic',
	'gs/outcom',
	'gs/utility',
	'gs/world',
	'net/io',
	'net/general',
	'mapgen',
	'sound',
	'ui/aui_common',
	'ui/aui_utils',
	'ui/aui_sdl',
	'ui/aui_directx',
	'ui/aui_ctp2',
	'ui/aui_common',
	'ui/interface',
	'ui/ldl',
	'ui/netshell',
	'ui/slic_debug',
	'ai',
	'ai/diplomacy',
	'ai/mapanalysis',
	'ai/strategy/scheduler',
	'ai/strategy/agents',
	'ai/strategy/goals',
	'ai/strategy/squads',
	'ai/CityManagement',
	'3rdparty'
)

game_core_sources = [
	'gs/core/game_observer.cpp',
	'gs/core/player_view.cpp',
	'gs/core/render_observer.cpp',
	'gfx/spritesys/director_render_observer.cpp',
	'gs/core/audio_observer.cpp',
	'sound/sound_manager_audio_observer.cpp',
	'gs/core/tiledmap_observer.cpp',
	'gfx/tilesys/tiledmap_observer_adapter.cpp',
	'gs/core/gfx_options_observer.cpp',
	'gfx/gfx_utils/gfx_options_observer_adapter.cpp',
	'gs/core/colorset_observer.cpp',
	'gfx/gfx_utils/colorset_observer_adapter.cpp',
	'gs/core/goodactor_factory.cpp',
	'gfx/spritesys/goodactor_factory_impl.cpp',
	'gs/core/splash_progress.cpp',
	'gs/core/progress_observer.cpp',
	'gs/core/text_observer.cpp',
	'gs/core/battle_observer.cpp',
	'gs/core/diplomacy_observer.cpp',
	'gs/fileio/json_save.cpp',
	'gs/gameobj/MovePath.cpp',
	'ctp/ctp2_utils/appstrings.cpp',
	'ctp/ctp2_utils/c3cmdline.cpp',
	'ctp/ctp2_utils/c3cpu.cpp',
	'ctp/ctp2_utils/c3debug.cpp',
	'ctp/ctp2_utils/c3errors.cpp',
	'ctp/ctp2_utils/c3files.cpp',
	'ctp/ctp2_utils/c3mem.cpp',
	'ctp/ctp2_utils/cheatkey.cpp',
	'ctp/ctp2_utils/ctpregistry.cpp',
	'ctp/ctp2_utils/ddhelp.cpp',
	'ctp/ctp2_utils/netconsole.cpp',
	'ctp/ctp2_utils/tracklen.cpp',
	'ctp/debugtools/debug.c',
	'ctp/debugtools/debugassert.cpp',
	'ctp/debugtools/debugcallstack.cpp',
	'ctp/debugtools/debugexception.cpp',
	'ctp/debugtools/debugmemory.cpp',
	'ctp/debugtools/log.cpp',
	'ctp/debugtools/Timer.cpp',
	'ctp/fingerprint/ctp_finger.cpp',
	'ctp/fingerprint/endecode.c',
	'ctp/fingerprint/shroud.c',
	'ctp/fingerprint/verify.c',
	'ctp/civapp.cpp',
	'ctp/display.cpp',
	'gs/database/DB.cpp',
	'gs/database/EndGameDB.cpp',
	'gs/database/EndGameRecord.cpp',
	'gs/database/filenamedb.cpp',
	'gs/database/highscoredb.cpp',
	'gs/database/moviedb.cpp',
	'gs/database/PlayListDB.cpp',
	'gs/database/profileDB.cpp',
	'gs/database/Rec.cpp',
	'gs/database/StrDB.cpp',
	'gs/database/StrRec.cpp',
	'gs/database/thronedb.cpp',
	'gs/database/UVDB.cpp',
	'gs/gameobj/AchievementTracker.cpp',
	'gs/gameobj/Advances.cpp',
	'gs/gameobj/advanceutil.cpp',
	'gs/gameobj/Agreement.cpp',
	'gs/gameobj/AgreementData.cpp',
	'gs/gameobj/AgreementPool.cpp',
	'gs/gameobj/Army.cpp',
	'gs/gameobj/ArmyData.cpp',
	'gs/gameobj/armyevent.cpp',
	'gs/gameobj/ArmyPool.cpp',
	'gs/gameobj/barbarians.cpp',
	'gs/gameobj/bldque.cpp',
	'gs/gameobj/buildingutil.cpp',
	'gs/gameobj/CityData.cpp',
	'gs/gameobj/CityEvent.cpp',
	'gs/gameobj/CityInfluenceIterator.cpp',
	'gs/gameobj/CityRadius.cpp',
	'gs/gameobj/Civilisation.cpp',
	'gs/gameobj/CivilisationData.cpp',
	'gs/gameobj/civilisationpool.cpp',
	'gs/gameobj/combatevent.cpp',
	'gs/gameobj/CriticalMessagesPrefs.cpp',
	'gs/gameobj/CTP2Combat.cpp',
	'gs/gameobj/Diffcly.cpp',
	'gs/gameobj/Diplomacy_Log.cpp',
	'gs/gameobj/DiplomaticRequest.cpp',
	'gs/gameobj/DiplomaticRequestData.cpp',
	'gs/gameobj/DiplomaticRequestPool.cpp',
	'gs/gameobj/EmptyTile.cpp',
	'gs/gameobj/endgame.cpp',
	'gs/gameobj/Events.cpp',
	'gs/gameobj/EventTracker.cpp',
	'gs/gameobj/Exclusions.cpp',
	'gs/gameobj/FeatTracker.cpp',
	'gs/gameobj/gaiacontroller.cpp',
	'gs/gameobj/GameObj.cpp',
	'gs/gameobj/GameSettings.cpp',
	'gs/gameobj/Gold.cpp',
	'gs/gameobj/GoodyHuts.cpp',
	'gs/gameobj/GSLogs.cpp',
	'gs/gameobj/Happy.cpp',
	'gs/gameobj/happytracker.cpp',
	'gs/gameobj/ID.cpp',
	'gs/gameobj/improvementevent.cpp',
	'gs/gameobj/installation.cpp',
	'gs/gameobj/installationdata.cpp',
	'gs/gameobj/installationpool.cpp',
	'gs/gameobj/MaterialPool.cpp',
	'gs/gameobj/Message.cpp',
	'gs/gameobj/messagedata.cpp',
	'gs/gameobj/MessagePool.cpp',
	'gs/gameobj/ObjPool.cpp',
	'gs/gameobj/Order.cpp',
	'gs/gameobj/Player.cpp',
	'gs/gameobj/PlayerEvent.cpp',
	'gs/gameobj/PlayerTurn.cpp',
	'gs/gameobj/PlayHap.cpp',
	'gs/gameobj/Pollution.cpp',
	'gs/gameobj/Readiness.cpp',
	'gs/gameobj/Regard.cpp',
	'gs/gameobj/Resources.cpp',
	'gs/gameobj/Sci.cpp',
	'gs/gameobj/Score.cpp',
	'gs/gameobj/Strengths.cpp',
	'gs/gameobj/TaxRate.cpp',
	'gs/gameobj/terrainutil.cpp',
	'gs/gameobj/TerrImprove.cpp',
	'gs/gameobj/TerrImproveData.cpp',
	'gs/gameobj/TerrImprovePool.cpp',
	'gs/gameobj/TopTen.cpp',
	'gs/gameobj/TradeBids.cpp',
	'gs/gameobj/tradeevent.cpp',
	'gs/gameobj/TradeOffer.cpp',
	'gs/gameobj/TradeOfferData.cpp',
	'gs/gameobj/TradeOfferPool.cpp',
	'gs/gameobj/TradePool.cpp',
	'gs/gameobj/TradeRoute.cpp',
	'gs/gameobj/TradeRouteData.cpp',
	'gs/gameobj/tradeutil.cpp',
	'gs/gameobj/Unit.cpp',
	'gs/gameobj/UnitData.cpp',
	'gs/gameobj/UnitState.cpp',
	'gs/gameobj/unitevent.cpp',
	'gs/gameobj/UnitPool.cpp',
	'gs/gameobj/unitutil.cpp',
	'gs/gameobj/Vision.cpp',
	'gs/gameobj/WonderTracker.cpp',
	'gs/gameobj/wonderutil.cpp',
	'gs/gameobj/Wormhole.cpp',
	'gs/gameobj/XY_Coordinates.cpp',
	lgen.process('gs/slic/sliccmd.l', extra_args : '-Psc'),
	pgen.process('gs/slic/sliccmd.y', extra_args : ['-p', 'sc']),
	lgen.process('gs/slic/slic.l', extra_args : ['-Pyy']),
	pgen.process('gs/slic/slic.y', extra_args : ['-p', 'yy']),
	'gs/slic/QuickSlic.cpp',
	'gs/slic/SlicArray.cpp',
	'gs/slic/SlicBuiltin.cpp',
	'gs/slic/SlicButton.cpp',
	'gs/slic/sliccmd.cpp',
	'gs/slic/SlicConditional.cpp',
	'gs/slic/SlicConst.cpp',
	'gs/slic/SlicContext.cpp',
	'gs/slic/SlicEngine.cpp',
	'gs/slic/SlicEyePoint.cpp',
	'gs/slic/SlicFrame.cpp',
	'gs/slic/slicfunc.cpp',
	'gs/slic/slicfuncai.cpp',
	'gs/slic/slicif.cpp',
	'gs/slic/sliciffile.cpp',
	'gs/slic/SlicModFunction.cpp',
	'gs/slic/SlicNamedSymbol.cpp',
	'gs/slic/slicobject.cpp',
	'gs/slic/SlicRecord.cpp',
	'gs/slic/SlicSegment.cpp',
	'gs/slic/SlicStack.cpp',
	'gs/slic/SlicStruct.cpp',
	'gs/slic/SlicSymbol.cpp',
	'gs/slic/SlicSymTab.cpp',
	'gs/utility/AgreementDynArr.cpp',
	'gs/utility/checksum.cpp',
	'gs/utility/CivDynArr.cpp',
	'gs/utility/clock.cpp',
	'gs/utility/DataCheck.cpp',
	'gs/utility/DiplDynArr.cpp',
	'gs/utility/ErrMsg.cpp',
	'gs/utility/gameinit.cpp',
	'gs/utility/MapFile.cpp',
	'gs/utility/MsgDynArr.cpp',
	'gs/utility/newturncount.cpp',
	'gs/utility/RandGen.cpp',
	'gs/utility/stringutils.cpp',
	'gs/utility/TradeDynArr.cpp',
	'gs/utility/TurnCnt.cpp',
	'gs/utility/TurnCntEvent.cpp',
	'gs/utility/UnitDynArr.cpp',
	'gs/world/Cell.cpp',
	'gs/world/cellunitlist.cpp',
	'gs/world/MapPoint.cpp',
	'gs/world/UnseenCell.cpp',
	'gs/world/wldgen.cpp',
	'gs/world/WorldDistance.cpp',
	'gs/world/worldevent.cpp',
	'gs/world/worldutils.cpp',
	'gs/world/WrldCity.cpp',
	'gs/world/WrldCont.cpp',
	'gs/world/WrldPoll.cpp',
	'gs/world/WrlEnv.cpp',
	'gs/world/WrlImprove.cpp',
	'gs/world/WrlUnit.cpp',
	'gs/fileio/CivPaths.cpp',
	'gs/fileio/civscenarios.cpp',
	'gs/fileio/GameFile.cpp',
	'gs/fileio/prjfile.cpp',
	'gs/fileio/Token.cpp',
	'gs/events/GameEvent.cpp',
	'gs/events/GameEventArgList.cpp',
	'gs/events/GameEventArgument.cpp',
	'gs/events/GameEventDescription.cpp',
	'gs/events/GameEventHook.cpp',
	'gs/events/GameEventManager.cpp',
	'gs/outcom/c3rand.cpp',
	'net/general/net_achievement.cpp',
	'net/general/net_action.cpp',
	'net/general/net_agreement.cpp',
	'net/general/net_army.cpp',
	'net/general/net_cell.cpp',
	'net/general/net_chat.cpp',
	'net/general/net_cheat.cpp',
	'net/general/net_city.cpp',
	'net/general/net_city.h',
	'net/general/net_civ.cpp',
	'net/general/net_crc.cpp',
	'net/general/net_diff.cpp',
	'net/general/net_diplomacy.cpp',
	'net/general/net_endgame.cpp',
	'net/general/net_exclusions.cpp',
	'net/general/net_feat.cpp',
	'net/general/net_gameobj.cpp',
	'net/general/net_gamesettings.cpp',
	'net/general/net_guid.cpp',
	'net/general/net_happy.cpp',
	'net/general/net_info.cpp',
	'net/general/net_installation.cpp',
	'net/general/net_keys.cpp',
	'net/general/net_message.cpp',
	'net/general/net_order.cpp',
	'net/general/net_player.cpp',
	'net/general/net_pollution.cpp',
	'net/general/net_rand.cpp',
	'net/general/net_ready.cpp',
	'net/general/net_report.cpp',
	'net/general/net_research.cpp',
	'net/general/net_strengths.cpp',
	'net/general/net_terrain.cpp',
	'net/general/net_tradeoffer.cpp',
	'net/general/net_traderoute.cpp',
	'net/general/net_unit.cpp',
	'net/general/net_vision.cpp',
	'net/general/net_wonder.cpp',
	'net/general/net_world.cpp',
	'net/general/network.cpp',
	'net/general/networkevent.cpp',
	'net/io/net_anet.cpp',
	'net/io/net_debug.cpp',
	'net/io/net_io.cpp',
	'net/io/net_thread.cpp',
	'robot/utility/RandTest.cpp',
	'robot/utility/roboinit.cpp',
	'robot/pathing/A_Star_Heuristic_Cost.cpp',
	'robot/pathing/astar.cpp',
	'robot/pathing/AstarPnt.cpp',
	'robot/pathing/AVLHeap.cpp',
	'robot/pathing/CityAstar.cpp',
	'robot/pathing/Path.cpp',
	'robot/pathing/robotastar2.cpp',
	'robot/pathing/TradeAstar.cpp',
	'robot/pathing/unitastar.cpp',
	'robot/aibackdoor/civarchive.cpp',
	'sound/civsound.cpp',
	'sound/gamesounds.cpp',
	'sound/soundevent.cpp',
	'sound/soundmanager.cpp',
	'sound/soundutils.cpp',
	'ui/aui_common/aui_action.cpp',
	'ui/aui_common/aui_animate.cpp',
	'ui/aui_common/aui_audiomanager.cpp',
	'ui/aui_common/aui_base.cpp',
	'ui/aui_common/aui_bitmapfont.cpp',
	'ui/aui_common/aui_blitter.cpp',
	'ui/aui_common/aui_button.cpp',
	'ui/aui_common/aui_control.cpp',
	'ui/aui_common/aui_cursor.cpp',
	'ui/aui_common/aui_dimension.cpp',
	'ui/aui_common/aui_dirtylist.cpp',
	'ui/aui_common/aui_dragdropwindow.cpp',
	'ui/aui_common/aui_dropdown.cpp',
	'ui/aui_common/aui_font.cpp',
	'ui/aui_common/aui_gamespecific.cpp',
	'ui/aui_common/aui_header.cpp',
	'ui/aui_common/aui_hypertextbase.cpp',
	'ui/aui_common/aui_hypertextbox.cpp',
	'ui/aui_common/aui_image.cpp',
	'ui/aui_common/aui_imagebase.cpp',
	'ui/aui_common/aui_imagelist.cpp',
	'ui/aui_common/aui_input.cpp',
	'ui/aui_common/aui_item.cpp',
	'ui/aui_common/aui_joystick.cpp',
	'ui/aui_common/aui_keyboard.cpp',
	'ui/aui_common/aui_ldl.cpp',
	'ui/aui_common/aui_listbox.cpp',
	'ui/aui_common/aui_memmap.cpp',
	'ui/aui_common/aui_mouse.cpp',
	'ui/aui_common/aui_movie.cpp',
	'ui/aui_common/aui_moviebutton.cpp',
	'ui/aui_common/aui_moviemanager.cpp',
	'ui/aui_common/aui_music.cpp',
	'ui/aui_common/aui_pixel.cpp',
	'ui/aui_common/aui_progressbar.cpp',
	'ui/aui_common/aui_radio.cpp',
	'ui/aui_common/aui_ranger.cpp',
	'ui/aui_common/aui_rectangle.cpp',
	'ui/aui_common/aui_region.cpp',
	'ui/aui_common/aui_regiondispatch.cpp',
	'ui/aui_common/aui_screen.cpp',
	'ui/aui_common/aui_shell.cpp',
	'ui/aui_common/aui_sound.cpp',
	'ui/aui_common/aui_soundbase.cpp',
	'ui/aui_common/aui_static.cpp',
	'ui/aui_common/aui_stringtable.cpp',
	'ui/aui_common/aui_surface.cpp',
	'ui/aui_common/aui_switch.cpp',
	'ui/aui_common/aui_switchgroup.cpp',
	'ui/aui_common/aui_tab.cpp',
	'ui/aui_common/aui_tabgroup.cpp',
	'ui/aui_common/aui_textbase.cpp',
	'ui/aui_common/aui_textbox.cpp',
	'ui/aui_common/aui_textfield.cpp',
	'ui/aui_common/aui_thumb.cpp',
	'ui/aui_common/aui_tipwindow.cpp',
	'ui/aui_common/aui_ui.cpp',
	'ui/aui_common/aui_undo.cpp',
	'ui/aui_common/aui_uniqueid.cpp',
	'ui/aui_common/aui_win.cpp',
	'ui/aui_common/aui_window.cpp',
	'ui/aui_common/tech_memmap.cpp',
	'ui/aui_common/aui_Factory.cpp',
	'ui/aui_utils/primitives.cpp',
	'ui/aui_utils/text_observer_adapter.cpp',
	'ui/aui_utils/textutils.cpp',
	'ui/aui_utils/vidplay.cpp',
	'ui/aui_directx/aui_directaudiomanager.cpp',
	'ui/aui_directx/aui_directblitter.cpp',
	'ui/aui_directx/aui_directinput.cpp',
	'ui/aui_directx/aui_directjoystick.cpp',
	'ui/aui_directx/aui_directkeyboard.cpp',
	'ui/aui_directx/aui_directmouse.cpp',
	'ui/aui_directx/aui_directmovie.cpp',
	'ui/aui_directx/aui_directmoviemanager.cpp',
	'ui/aui_directx/aui_directsound.cpp',
	'ui/aui_directx/aui_directsurface.cpp',
	'ui/aui_directx/aui_directui.cpp',
	'ui/aui_directx/aui_directx.cpp',
	'ui/aui_sdl/aui_sdl.cpp',
	'ui/aui_sdl/aui_sdlblitter.cpp',
	'ui/aui_sdl/aui_sdlinput.cpp',
	'ui/aui_sdl/aui_sdlinput.h',
	'ui/aui_sdl/aui_sdljoystick.cpp',
	'ui/aui_sdl/aui_sdlkeyboard.cpp',
	'ui/aui_sdl/aui_sdlmouse.cpp',
	'ui/aui_sdl/aui_sdlsound.cpp',
	'ui/aui_sdl/aui_sdlsurface.cpp',
	'ui/aui_sdl/aui_sdlui.cpp',
	'ui/aui_ctp2/background.cpp',
	'ui/aui_ctp2/battleorderbox.cpp',
	'ui/aui_ctp2/battleorderboxactions.cpp',
	'ui/aui_ctp2/bevellesswindow.cpp',
	'ui/aui_ctp2/c3_button.cpp',
	'ui/aui_ctp2/c3_checkbox.cpp',
	'ui/aui_ctp2/c3_coloredstatic.cpp',
	'ui/aui_ctp2/c3_coloredswitch.cpp',
	'ui/aui_ctp2/c3_coloriconbutton.cpp',
	'ui/aui_ctp2/c3_dropdown.cpp',
	'ui/aui_ctp2/c3_fancywindow.cpp',
	'ui/aui_ctp2/c3_header.cpp',
	'ui/aui_ctp2/c3_headerswitch.cpp',
	'ui/aui_ctp2/c3_hypertextbox.cpp',
	'ui/aui_ctp2/c3_hypertipwindow.cpp',
	'ui/aui_ctp2/c3_icon.cpp',
	'ui/aui_ctp2/c3_image.cpp',
	'ui/aui_ctp2/c3_listbox.cpp',
	'ui/aui_ctp2/c3_listitem.cpp',
	'ui/aui_ctp2/c3_popupwindow.cpp',
	'ui/aui_ctp2/c3_radio.cpp',
	'ui/aui_ctp2/c3_ranger.cpp',
	'ui/aui_ctp2/c3_slidometer.cpp',
	'ui/aui_ctp2/c3_static.cpp',
	'ui/aui_ctp2/c3_switch.cpp',
	'ui/aui_ctp2/c3_thumb.cpp',
	'ui/aui_ctp2/c3_updateaction.cpp',
	'ui/aui_ctp2/c3_utilitydialogbox.cpp',
	'ui/aui_ctp2/c3blitter.cpp',
	'ui/aui_ctp2/c3dropdown.cpp',
	'ui/aui_ctp2/c3fancywindow.cpp',
	'ui/aui_ctp2/c3imageformats.cpp',
	'ui/aui_ctp2/c3listbox.cpp',
	'ui/aui_ctp2/c3memmap.cpp',
	'ui/aui_ctp2/c3scroller.cpp',
	'ui/aui_ctp2/C3slider.cpp',
	'ui/aui_ctp2/c3spinner.cpp',
	'ui/aui_ctp2/c3textfield.cpp',
	'ui/aui_ctp2/c3thumb.cpp',
	'ui/aui_ctp2/c3ui.cpp',
	'ui/aui_ctp2/c3window.cpp',
	'ui/aui_ctp2/c3windows.cpp',
	'ui/aui_ctp2/chart.cpp',
	'ui/aui_ctp2/checkbox.cpp',
	'ui/aui_ctp2/cityinventorylistbox.cpp',
	'ui/aui_ctp2/coloriconbutton.cpp',
	'ui/aui_ctp2/coloriconswitch.cpp',
	'ui/aui_ctp2/controlsheet.cpp',
	'ui/aui_ctp2/directvideo.cpp',
	'ui/aui_ctp2/grabitem.cpp',
	'ui/aui_ctp2/icon.cpp',
	'ui/aui_ctp2/iconbutton.cpp',
	'ui/aui_ctp2/iconswitch.cpp',
	'ui/aui_ctp2/icontab.cpp',
	'ui/aui_ctp2/InfoBar.cpp',
	'ui/aui_ctp2/iteminfo.cpp',
	'ui/aui_ctp2/keymap.cpp',
	'ui/aui_ctp2/keypress.cpp',
	'ui/aui_ctp2/linegraph.cpp',
	'ui/aui_ctp2/pattern.cpp',
	'ui/aui_ctp2/patternbase.cpp',
	'ui/aui_ctp2/picture.cpp',
	'ui/aui_ctp2/picturebutton.cpp',
	'ui/aui_ctp2/pictureswitch.cpp',
	'ui/aui_ctp2/picturetab.cpp',
	'ui/aui_ctp2/radarmap.cpp',
	'ui/aui_ctp2/radiogroup.cpp',
	'ui/aui_ctp2/SelItem.cpp',
	'ui/aui_ctp2/SelItemClick.cpp',
	'ui/aui_ctp2/SelItemEvent.cpp',
	'ui/aui_ctp2/staticpicture.cpp',
	'ui/aui_ctp2/statictextitem.cpp',
	'ui/aui_ctp2/statuswindow.cpp',
	'ui/aui_ctp2/textbox.cpp',
	'ui/aui_ctp2/textbutton.cpp',
	'ui/aui_ctp2/textradio.cpp',
	'ui/aui_ctp2/textswitch.cpp',
	'ui/aui_ctp2/texttab.cpp',
	'ui/aui_ctp2/thermometer.cpp',
	'ui/aui_ctp2/thronecontrol.cpp',
	'ui/aui_ctp2/thumbnailmap.cpp',
	'ui/aui_ctp2/tilecontrol.cpp',
	'ui/aui_ctp2/tipwindow.cpp',
	'ui/aui_ctp2/unittabbutton.cpp',
	'ui/aui_ctp2/videowindow.cpp',
	'ui/aui_ctp2/ctp2_button.cpp',
	'ui/aui_ctp2/ctp2_commandline.cpp',
	'ui/aui_ctp2/ctp2_dropdown.cpp',
	'ui/aui_ctp2/ctp2_hypertextbox.cpp',
	'ui/aui_ctp2/ctp2_listbox.cpp',
	'ui/aui_ctp2/ctp2_listitem.cpp',
	'ui/aui_ctp2/ctp2_Menu.cpp',
	'ui/aui_ctp2/ctp2_menubar.cpp',
	'ui/aui_ctp2/ctp2_MenuButton.cpp',
	'ui/aui_ctp2/ctp2_spinner.cpp',
	'ui/aui_ctp2/ctp2_Static.cpp',
	'ui/aui_ctp2/ctp2_Switch.cpp',
	'ui/aui_ctp2/ctp2_Tab.cpp',
	'ui/aui_ctp2/ctp2_TabButton.cpp',
	'ui/aui_ctp2/ctp2_TabGroup.cpp',
	'ui/aui_ctp2/ctp2_textbuffer.cpp',
	'ui/aui_ctp2/ctp2_textfield.cpp',
	'ui/aui_ctp2/ctp2_Window.cpp',
	'ui/aui_ctp2/player_view_ui.cpp',
	'ui/aui_ctp2/ui_events.cpp',
	'ui/aui_ctp2/ui_game_observer.cpp',
	'ui/aui_ctp2/ui_unit_actor_registry.cpp',
	'ui/interface/CityControlPanel.cpp',
	'ui/interface/DomesticControlPanel.cpp',
	'ui/interface/MessageControlPanel.cpp',
	'ui/interface/TilesControlPanel.cpp',
	'ui/interface/UnitControlPanel.cpp',
	'ui/interface/ControlTabPanel.cpp',
	'ui/interface/EndTurnButton.cpp',
	'ui/interface/MainControlPanel.cpp',
	'ui/interface/ShortcutPad.cpp',
	'ui/interface/StatusBar.cpp',
	'ui/interface/SpecialAttackWindow.cpp',
	'ui/interface/TurnYearStatus.cpp',
	'ui/interface/ZoomPad.cpp',
	'ui/interface/controlpanelwindow.cpp',
	'ui/interface/interfaceevent.cpp',
	'ui/interface/agesscreen.cpp',
	'ui/interface/ancientwindows.cpp',
	'ui/interface/armymanagerwindow.cpp',
	'ui/interface/AttractWindow.cpp',
	'ui/interface/backgroundwin.cpp',
	'ui/interface/c3dialogs.cpp',
	'ui/interface/chatbox.cpp',
	'ui/interface/CityEspionage.cpp',
	'ui/interface/citymanager.cpp',
	'ui/interface/creditsscreen.cpp',
	'ui/interface/cursormanager.cpp',
	'ui/interface/custommapscreen.cpp',
	'ui/interface/debugwindow.cpp',
	'ui/interface/DiplomacyDetails.cpp',
	'ui/interface/diplomacywindow.cpp',
	'ui/interface/dipwizard.cpp',
	'ui/interface/EditQueue.cpp',
	'ui/interface/EndgameWindow.cpp',
	'ui/interface/FileDialog.cpp',
	'ui/interface/gameplayoptions.cpp',
	'ui/interface/graphicsresscreen.cpp',
	'ui/interface/graphicsscreen.cpp',
	'ui/interface/greatlibrary.cpp',
	'ui/interface/greatlibrary_util.cpp',
	'ui/interface/greatlibrarywindow.cpp',
	'ui/interface/helptile.cpp',
	'ui/interface/hotseatlist.cpp',
	'ui/interface/infowin.cpp',
	'ui/interface/initialplayscreen.cpp',
	'ui/interface/initialplaywindow.cpp',
	'ui/interface/intelligencewindow.cpp',
	'ui/interface/IntroMovieWin.cpp',
	'ui/interface/IntroMovieWindow.cpp',
	'ui/interface/km_screen.cpp',
	'ui/interface/loadsavemapscreen.cpp',
	'ui/interface/loadsavemapwindow.cpp',
	'ui/interface/loadsavescreen.cpp',
	'ui/interface/loadsavewindow.cpp',
	'ui/interface/MapCopyBuffer.cpp',
	'ui/interface/messageactions.cpp',
	'ui/interface/messageadvice.cpp',
	'ui/interface/messageeyepoint.cpp',
	'ui/interface/messageiconbutton.cpp',
	'ui/interface/messageiconwindow.cpp',
	'ui/interface/messagelist.cpp',
	'ui/interface/messagemodal.cpp',
	'ui/interface/messageresponse.cpp',
	'ui/interface/messagewin.cpp',
	'ui/interface/messagewindow.cpp',
	'ui/interface/musicscreen.cpp',
	'ui/interface/musictrackscreen.cpp',
	'ui/interface/optionsscreen.cpp',
	'ui/interface/optionswindow.cpp',
	'ui/interface/optionwarningscreen.cpp',
	'ui/interface/ProfileEdit.cpp',
	'ui/interface/progresswindow.cpp',
	'ui/interface/progresswindow_observer_adapter.cpp',
	'ui/interface/radarwindow.cpp',
	'ui/interface/scenarioeditor.cpp',
	'ui/interface/scenarioscreen.cpp',
	'ui/interface/scenariowindow.cpp',
	'ui/interface/sciencewin.cpp',
	'ui/interface/scorewarn.cpp',
	'ui/interface/screenutils.cpp',
	'ui/interface/soundscreen.cpp',
	'ui/interface/splash.cpp',
	'ui/interface/spnewgamediffscreen.cpp',
	'ui/interface/spnewgamemapshapescreen.cpp',
	'ui/interface/spnewgamemapsizescreen.cpp',
	'ui/interface/spnewgameplayersscreen.cpp',
	'ui/interface/spnewgamerandomcustomscreen.cpp',
	'ui/interface/spnewgamerulesscreen.cpp',
	'ui/interface/spnewgamescreen.cpp',
	'ui/interface/spnewgametribescreen.cpp',
	'ui/interface/spnewgamewindow.cpp',
	'ui/interface/spriteeditor.cpp',
	'ui/interface/statictextbox.cpp',
	'ui/interface/statswindow.cpp',
	'ui/interface/String_Search.cpp',
	'ui/interface/tileimptracker.cpp',
	'ui/interface/trademanager.cpp',
	'ui/interface/tutorialwin.cpp',
	'ui/interface/UIUtils.cpp',
	'ui/interface/unitmanager.cpp',
	'ui/interface/victorymoviewin.cpp',
	'ui/interface/victorymoviewindow.cpp',
	'ui/interface/victorywin.cpp',
	'ui/interface/victorywindow.cpp',
	'ui/interface/wondermoviewin.cpp',
	'ui/interface/wondermoviewindow.cpp',
	'ui/interface/WonderTab.cpp',
	'ui/interface/workwin.cpp',
	'ui/interface/workwindow.cpp',
	'ui/interface/InterfaceEventRegister.cpp',
	'ui/interface/CauseAndEffectTab.cpp',
	'ui/interface/DomesticManagementDialog.cpp',
	'ui/interface/GovernmentTab.cpp',
	'ui/interface/citywindow.cpp',
	'ui/interface/sci_advancescreen.cpp',
	'ui/interface/ScienceManagementDialog.cpp',
	'ui/interface/NationalManagementDialog.cpp',
	'ui/interface/MessageBoxDialog.cpp',
	'ui/interface/battle.cpp',
	'ui/interface/battleevent.cpp',
	'ui/interface/battleview.cpp',
	'ui/interface/battleviewwindow.cpp',
	'ui/interface/battle_observer_adapter.cpp',
	'ui/interface/dipwizard_observer_adapter.cpp',
	'ui/interface/infowindow.cpp',
	'ui/interface/rankingtab.cpp',
	'ui/interface/scoretab.cpp',
	'ui/interface/timelinetab.cpp',
	'ui/interface/sciencevictorydialog.cpp',
	lgen.process('ui/ldl/ldl.l', extra_args : '-Pldl'),
	pgen.process('ui/ldl/ldl.y', extra_args : ['-p', 'ldl']),
	'ui/ldl/ldl_attr.cpp',
	'ui/ldl/ldl_data.cpp',
	'ui/ldl/ldl_data_info.cpp',
	'ui/ldl/ldl_file.cpp',
	'ui/ldl/ldl_memmap.cpp',
	'ui/ldl/ldlif.cpp',
	'ui/netshell/allinonewindow.cpp',
	'ui/netshell/connectionselectwindow.cpp',
	'ui/netshell/dialogboxwindow.cpp',
	'ui/netshell/gameselectwindow.cpp',
	'ui/netshell/lobbychangewindow.cpp',
	'ui/netshell/lobbywindow.cpp',
	'ui/netshell/netfunc.cpp',
	'ui/netshell/netshell.cpp',
	'ui/netshell/netshell_game.cpp',
	'ui/netshell/ns_aiplayer.cpp',
	'ui/netshell/ns_aiplayersetup.cpp',
	'ui/netshell/ns_chatbox.cpp',
	'ui/netshell/ns_civlistbox.cpp',
	'ui/netshell/ns_customlistbox.cpp',
	'ui/netshell/ns_game.cpp',
	'ui/netshell/ns_gamesetup.cpp',
	'ui/netshell/ns_gplayer.cpp',
	'ui/netshell/ns_header.cpp',
	'ui/netshell/ns_headerswitch.cpp',
	'ui/netshell/ns_improvements.cpp',
	'ui/netshell/ns_item.cpp',
	'ui/netshell/ns_lobby.cpp',
	'ui/netshell/ns_player.cpp',
	'ui/netshell/ns_playersetup.cpp',
	'ui/netshell/ns_rplayer.cpp',
	'ui/netshell/ns_server.cpp',
	'ui/netshell/ns_session.cpp',
	'ui/netshell/ns_spinner.cpp',
	'ui/netshell/ns_string.cpp',
	'ui/netshell/ns_transport.cpp',
	'ui/netshell/ns_tribes.cpp',
	'ui/netshell/ns_units.cpp',
	'ui/netshell/ns_window.cpp',
	'ui/netshell/ns_wonders.cpp',
	'ui/netshell/passwordscreen.cpp',
	'ui/netshell/playereditwindow.cpp',
	'ui/netshell/playerselectwindow.cpp',
	'ui/netshell/serverselectwindow.cpp',
	'ui/slic_debug/segmentlist.cpp',
	'ui/slic_debug/sourcelist.cpp',
	'ui/slic_debug/watchlist.cpp',
	'ai/diplomacy/counterresponseevent.cpp',
	'ai/diplomacy/dstateevent.cpp',
	'ai/diplomacy/motivationevent.cpp',
	'ai/diplomacy/nproposalevent.cpp',
	'ai/diplomacy/proposalresponseevent.cpp',
	'ai/diplomacy/ReactEvent.cpp',
	'ai/diplomacy/regardevent.cpp',
	'ai/diplomacy/RejectResponseEvent.cpp',
	'ai/diplomacy/ResponseEvent.cpp',
	'ai/diplomacy/sstateevent.cpp',
	'ai/diplomacy/threatresponseevent.cpp',
	'ai/diplomacy/agreementmatrix.cpp',
	'ai/diplomacy/diplomacyutil.cpp',
	'ai/diplomacy/diplomat.cpp',
	'gs/diplomacy/diplomacy_types.cpp',
	'ai/diplomacy/foreigner.cpp',
	'ai/diplomacy/proposalanalysis.cpp',
	'ai/strategy/scheduler/Plan.cpp',
	'ai/strategy/scheduler/scheduler.cpp',
	'ai/strategy/goals/Goal.cpp',
	'ai/strategy/squads/squad_Strength.cpp',
	'ai/strategy/agents/agent.cpp',
	'ai/mapanalysis/boundingrect.cpp',
	'ai/mapanalysis/mapanalysis.cpp',
	'ai/mapanalysis/settlemap.cpp',
	'ai/CityManagement/governor.cpp',
	'ai/ctpai.cpp',
	'ai/ctpaidebug.cpp',
	'mapgen/Crater.cpp',
	'mapgen/FaultGen.cpp',
	'mapgen/Geometric.cpp',
	'mapgen/PlasmaGen2.cpp',
]

# Entry-point sources — added only to the game executable.
# ctp/civ3_main.cpp is included in the test target with -DUNIT_TEST_BUILD
# to suppress its main() function while keeping global definitions.
game_entry_sources = [
	'ctp/civ3_main.cpp',
	'test/smoketest_server.cpp',
]

main_sources = game_core_sources + game_entry_sources

ctpdb = executable('ctpdb', [
	'gs/dbgen/Datum.cpp',
	'gs/dbgen/MemberClass.cpp',
	'gs/dbgen/RecordDescription.cpp',
	'gs/dbgen/ctpdb.cpp',
	lgen.process('gs/dbgen/ctpdb.l', extra_args : '-Pyy'),
	pgen.process('gs/dbgen/ctpdb.y', extra_args : ['-p', 'yy'])
], 
	include_directories : [ incdir, 
	include_directories([ 'gs/dbgen', 'os/include' ])],
		cpp_args : '-fpermissive'
)

cdb_sources = [
	custom_target('advance',
			input : 'gs/newdb/advance.cdb',
			output : ['AdvanceRecord.cpp', 'AdvanceRecord.h',
						'AdvanceBranchRecord.cpp', 'AdvanceBranchRecord.h',
						'AgeRecord.cpp', 'AgeRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
#	custom_target('age',
#			input : 'gs/newdb/age.cdb',
#			output : ['AgeRecord.cpp', 'AgeRecord.h',
#						'AgeCityTerrainRecord.cpp', 'AgeCityTerrainRecord.h',
#						'AgeCityStyleRecord.cpp', 'AgeCityStyleRecord.h']
#			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('building',
			input : 'gs/newdb/building.cdb',
			output : ['BuildingRecord.cpp', 'BuildingRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('citysize',
			input : 'gs/newdb/citysize.cdb',
			output : ['CitySizeRecord.cpp', 'CitySizeRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('citystyle',
			input : 'gs/newdb/citystyle.cdb',
			output : ['AgeCityStyleRecord.cpp', 'AgeCityStyleRecord.h',
						'CityStyleRecord.cpp', 'CityStyleRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Civilisation',
			input : 'gs/newdb/Civilisation.cdb',
			output : ['CivilisationRecord.cpp', 'CivilisationRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Concept',
			input : 'gs/newdb/Concept.cdb',
			output : ['ConceptRecord.cpp', 'ConceptRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Const',
			input : 'gs/newdb/Const.cdb',
			output : ['ConstRecord.cpp', 'ConstRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('culture',
			input : 'gs/newdb/culture.cdb',
			output : ['CultureRecord.cpp', 'CultureRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Difficulty',
			input : 'gs/newdb/Difficulty.cdb',
			output : ['DifficultyRecord.cpp', 'DifficultyRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('diplomacy',
			input : 'gs/newdb/diplomacy.cdb',
			output : ['DiplomacyRecord.cpp', 'DiplomacyRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('DiplomacyProposal',
			input : 'gs/newdb/DiplomacyProposal.cdb',
			output : ['DiplomacyProposalRecord.cpp', 'DiplomacyProposalRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('DiplomacyThreat',
			input : 'gs/newdb/DiplomacyThreat.cdb',
			output : ['DiplomacyThreatRecord.cpp', 'DiplomacyThreatRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('EndGameObject',
			input : 'gs/newdb/EndGameObject.cdb',
			output : ['EndGameObjectRecord.cpp', 'EndGameObjectRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('feat',
			input : 'gs/newdb/feat.cdb',
			output : ['FeatRecord.cpp', 'FeatRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('GlobalWarming',
			input : 'gs/newdb/GlobalWarming.cdb',
			output : ['GlobalWarmingRecord.cpp', 'GlobalWarmingRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('goal',
			input : 'gs/newdb/goal.cdb',
			output : ['GoalRecord.cpp', 'GoalRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('government',
			input : 'gs/newdb/government.cdb',
			output : ['GovernmentRecord.cpp', 'GovernmentRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('icon',
			input : 'gs/newdb/icon.cdb',
			output : ['IconRecord.cpp', 'IconRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Leader',
			input : 'gs/newdb/Leader.cdb',
			output : ['LeaderRecord.cpp', 'LeaderRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Map',
			input : 'gs/newdb/Map.cdb',
			output : ['MapRecord.cpp', 'MapRecord.h', 
						'MapIconRecord.cpp', 'MapIconRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('order',
			input : 'gs/newdb/order.cdb',
			output : ['OrderRecord.cpp', 'OrderRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('personality',
			input : 'gs/newdb/personality.cdb',
			output : ['PersonalityRecord.cpp', 'PersonalityRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Pollution',
			input : 'gs/newdb/Pollution.cdb',
			output : ['PollutionRecord.cpp', 'PollutionRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('pop',
			input : 'gs/newdb/pop.cdb',
			output : ['PopRecord.cpp', 'PopRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('religion',
			input : 'gs/newdb/religion.cdb',
			output : ['ReligionRecord.cpp', 'ReligionRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('Risk',
			input : 'gs/newdb/Risk.cdb',
			output : ['RiskRecord.cpp', 'RiskRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('strategy',
			input : 'gs/newdb/strategy.cdb',
			output : ['BuildListSequenceRecord.cpp', 'BuildListSequenceRecord.h',
						'ImprovementListRecord.cpp', 'ImprovementListRecord.h',
						'BuildingBuildListRecord.cpp', 'BuildingBuildListRecord.h',
						'StrategyRecord.cpp', 'StrategyRecord.h',
						'AdvanceListRecord.cpp', 'AdvanceListRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('terrain',
			input : 'gs/newdb/terrain.cdb',
			output : ['ResourceRecord.cpp', 'ResourceRecord.h',
						'TerrainRecord.cpp', 'TerrainRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('TerrainImprovement',
			input : 'gs/newdb/TerrainImprovement.cdb',
			output : ['TerrainImprovementRecord.cpp', 'TerrainImprovementRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('unit',
			input : 'gs/newdb/unit.cdb',
			output : ['SoundRecord.cpp', 'SoundRecord.h',
						'SpecialAttackInfoRecord.cpp', 'SpecialAttackInfoRecord.h',
						'SpecialEffectRecord.cpp', 'SpecialEffectRecord.h',
						'SpriteRecord.cpp', 'SpriteRecord.h',
						'UnitBuildListRecord.cpp', 'UnitBuildListRecord.h',
						'UnitRecord.cpp', 'UnitRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('unitpromotion',
			input : 'gs/newdb/unitpromotion.cdb',
			output : ['UnitpromotionRecord.cpp', 'UnitpromotionRecord.h'],
			command : [ctpdb, '-i', '@INPUT@']),
	custom_target('wonder',
			input : 'gs/newdb/wonder.cdb',
			output : ['WonderBuildListRecord.cpp', 'WonderBuildListRecord.h',
						'WonderRecord.cpp', 'WonderRecord.h',
						'WonderMovieRecord.cpp', 'WonderMovieRecord.h'],
			command : [ctpdb, '-i', '@INPUT@'])
]

cdb_header = declare_dependency(sources : cdb_sources, dependencies : freetype)

gsnewdb = static_library('libgsnewdb', [
	cdb_sources,
	lgen.process('gs/newdb/DBLexer.l', extra_args : ['-Pdbl']),
	'gs/newdb/CTPDatabase.cpp',
	'gs/newdb/CTPRecord.cpp',
	'gs/newdb/DBLexer.cpp'],
		include_directories : incdir,
		dependencies : freetype)

gfxutils = static_library('libgfxgfx_utils', [
	'gfx/gfx_utils/arproces.cpp',
	'gfx/gfx_utils/ColorSet.cpp',
	'gfx/gfx_utils/gfx_options.cpp',
	'gfx/gfx_utils/pixelutils.cpp',
	'gfx/gfx_utils/targautils.cpp',
	'gfx/gfx_utils/tiffutils.cpp',
	'gfx/gfx_utils/videoutils.cpp'],
		include_directories : incdir,
		dependencies : [ cdb_header, tiff, sdl ])

gfxlayers = static_library('libgfxlayers', 'gfx/layers/citylayer.cpp',
		include_directories : incdir,
		dependencies : [ cdb_header, sdl ])

gfxspritesys = static_library('libgfxspritesys', [
	'gfx/spritesys/action.cpp',
	'gfx/spritesys/Actor.cpp',
	'gfx/spritesys/ActorPath.cpp',
	'gfx/spritesys/Anim.cpp',
	'gfx/spritesys/battleviewactor.cpp',
	'gfx/spritesys/DirectorActions.cpp',
	'gfx/spritesys/directorevent.cpp',
	'gfx/spritesys/EffectActor.cpp',
	'gfx/spritesys/goodactor.cpp',
	'gfx/spritesys/TradeActor.cpp',
	'gfx/spritesys/UnitActor.cpp',
	'gfx/spritesys/workeractor.cpp',
	'gfx/spritesys/CitySpriteGroup.cpp',
	'gfx/spritesys/effectspritegroup.cpp',
	'gfx/spritesys/FacedSprite.cpp',
	'gfx/spritesys/FacedSpriteWshadow.cpp',
	'gfx/spritesys/goodspritegroup.cpp',
	'gfx/spritesys/Sprite.cpp',
	'gfx/spritesys/spritefile.cpp',
	'gfx/spritesys/SpriteGroup.cpp',
	'gfx/spritesys/SpriteGroupList.cpp',
	'gfx/spritesys/SpriteHitTest.cpp',
	'gfx/spritesys/SpriteList.cpp',
	'gfx/spritesys/SpriteLow555.cpp',
	'gfx/spritesys/SpriteLow565.cpp',
	'gfx/spritesys/SpriteState.cpp',
	'gfx/spritesys/spriteutils.cpp',
	'gfx/spritesys/UnitSpriteGroup.cpp',
	'gfx/spritesys/ActionQueue.cpp',
	'gfx/spritesys/director.cpp',
	'gfx/spritesys/DirectorActionHandlers.cpp',
	'gfx/spritesys/screenmanager.cpp',
	'gfx/spritesys/Sequence.cpp',
	'gfx/spritesys/SpriteStateDB.cpp',
	'gfx/spritesys/spritelow.cpp'],
		include_directories : incdir,
		dependencies : [ cdb_header, anet, sdl ])

gfxtilesys = static_library('libgfxtilesys', [
	'gfx/tilesys/maputils.cpp',
	'gfx/tilesys/BaseTile.cpp',
	'gfx/tilesys/resourcemap.cpp',
	'gfx/tilesys/tiledmap.cpp',
	'gfx/tilesys/tiledraw.cpp',
	'gfx/tilesys/tiledrawroad.cpp',
	'gfx/tilesys/TileHighlight.cpp',
	'gs/world/TileInfo.cpp',
	'gfx/tilesys/tileset.cpp',
	'gfx/tilesys/tileutils.cpp',
	'gfx/tilesys/workmap.cpp'],
		include_directories : incdir,
		dependencies : [ cdb_header, anet, sdl ])

# mapgen plugins
p_crater = shared_library('crater', 'mapgen/Crater.cpp', 
		include_directories : incdir)

p_fault = shared_library('fault', 'mapgen/FaultGen.cpp', 
		include_directories : incdir)

p_geometric = shared_library('geometric', 'mapgen/Geometric.cpp',
		include_directories : incdir,
		dependencies : freetype,
		cpp_args : '-DGEOMETRIC',
		link_with : p_fault)

p_plasma2 = shared_library('plasma2', 'mapgen/PlasmaGen2.cpp',
		include_directories : incdir)

# main exe
executable('ctp2', main_sources,
		include_directories : incdir,
		dependencies : [ cdb_header, anet, freetype, main_dependencies ],
		cpp_args : [ '-fpermissive', '-Wno-narrowing', '-Wno-non-pod-varargs' ],
		link_args : [ '-lltdl', use_ld ],
		link_with : [ gsnewdb, gfxutils, gfxlayers, gfxspritesys, gfxtilesys ])

# headless exe — simulation core without UI/audio/render
# Same sources as ctp2 for now; stubs out UI at runtime via g_headlessMode.
# Future: split game_core_sources into engine_sources + ui_sources.
headless_sources = game_core_sources + [
	'ctp/headless_main.cpp',
	'ctp/headless_game_observer.cpp',
	'ctp/civ3_main.cpp',
	'test/smoketest_server.cpp',
]

executable('ctp2_headless', headless_sources,
		include_directories : incdir,
		dependencies : [ cdb_header, anet, freetype, main_dependencies ],
		cpp_args : [ '-fpermissive', '-Wno-narrowing', '-Wno-non-pod-varargs', '-DHEADLESS_BUILD', '-DUNIT_TEST_BUILD' ],
		link_args : [ '-lltdl', use_ld ],
		link_with : [ gsnewdb, gfxutils, gfxlayers, gfxspritesys, gfxtilesys ])

# unit tests
doctest_incdir = include_directories('test/cpp')

# ---------------------------------------------------------------------------
# Fast test runner — observer + pure unit tests only.
# These tests have zero global state dependencies, run in < 100 ms, and never
# crash.  Intended for `make test` after every edit.
# ---------------------------------------------------------------------------
cpp_fast_test_sources = [
  'test/cpp/main.cpp',
  'test/cpp/test_observer.cpp',
  'test/cpp/test_tiledmap_observer.cpp',
  'test/cpp/test_gfx_options_observer.cpp',
  'test/cpp/test_audio_observer.cpp',
  'test/cpp/test_colorset_observer.cpp',
  'test/cpp/test_color_types.cpp',
  'test/cpp/test_render_observer.cpp',
  'test/cpp/test_randgen.cpp',
  'test/cpp/test_scoped_rand.cpp',
  'test/cpp/test_mappoint.cpp',
  'test/cpp/test_army.cpp',
  'test/cpp/test_bugfixes.cpp',
  'test/cpp/test_player_view.cpp',
  'test/cpp/test_ui_command_surface.cpp',
  'ctp/civ3_main.cpp',
  'test/smoketest_server.cpp',
]

fast_test_exe = executable('ctp2_fast_tests', cpp_fast_test_sources + game_core_sources,
  include_directories : [ incdir, doctest_incdir ],
  dependencies : [ cdb_header, anet, freetype, main_dependencies ],
  cpp_args : [ '-fpermissive', '-Wno-narrowing', '-Wno-non-pod-varargs', '-DUNIT_TEST_BUILD' ],
  link_args : [ '-lltdl', use_ld ],
  link_with : [ gsnewdb, gfxutils, gfxlayers, gfxspritesys, gfxtilesys ],
)

# meson.build lives in ctp2_code/, but data files (appstr.txt, etc.) are in the parent dir
cwd_for_tests = join_paths(meson.project_source_root(), '..')

test('fast', fast_test_exe,
  args : ['--duration=true'],
  timeout : 60,
  workdir : cwd_for_tests,
)

# ---------------------------------------------------------------------------
# Full unit-test suite — adds database-dependent and integration tests
# (BuildQueue + CityData load the real game DBs; headless_* spawn the
# ctp2_headless binary).  Total ~30 s.  Run with `make test-full` or
# `meson test`.
# ---------------------------------------------------------------------------
cpp_test_sources = [
  'test/cpp/main.cpp',
  'test/cpp/test_randgen.cpp',
  'test/cpp/test_scoped_rand.cpp',
  'test/cpp/test_mappoint.cpp',
  'test/cpp/test_army.cpp',
  'test/cpp/test_citydata.cpp',
  'test/cpp/test_buildqueue.cpp',
  'test/cpp/test_happy.cpp',
  'test/cpp/test_bugfixes.cpp',
  'test/cpp/test_observer.cpp',
  'test/cpp/test_tiledmap_observer.cpp',
  'test/cpp/test_gfx_options_observer.cpp',
  'test/cpp/test_audio_observer.cpp',
  'test/cpp/test_colorset_observer.cpp',
  'test/cpp/test_color_types.cpp',
  'test/cpp/test_render_observer.cpp',
  'test/cpp/test_player_view.cpp',
  'test/cpp/test_ui_command_surface.cpp',
  'test/cpp/test_headless_smoke.cpp',
  'test/cpp/test_headless_determinism.cpp',
  'test/cpp/test_save_determinism.cpp',
  'test/cpp/test_json_save.cpp',
  'test/cpp/test_json_save_integration.cpp',
  'test/cpp/test_json_world.cpp',
  'test/cpp/test_save_load.cpp',
  'test/cpp/test_headless_progression.cpp',
  'test/cpp/test_headless_long.cpp',
  'ctp/civ3_main.cpp',
  'test/smoketest_server.cpp',
]

unit_test_exe = executable('ctp2_unit_tests', cpp_test_sources + game_core_sources,
  include_directories : [ incdir, doctest_incdir ],
  dependencies : [ cdb_header, anet, freetype, main_dependencies ],
  cpp_args : [ '-fpermissive', '-Wno-narrowing', '-Wno-non-pod-varargs', '-DUNIT_TEST_BUILD' ],
  link_args : [ '-lltdl', use_ld ],
  link_with : [ gsnewdb, gfxutils, gfxlayers, gfxspritesys, gfxtilesys ],
)

test('unit', unit_test_exe,
  # Default unit run excludes the headless-subprocess "integration" suite
  # (test_headless_*.cpp + test_save_load.cpp) which runs real game turns
  # and takes ~73s.  Run those separately via the `integration` target
  # below (or with --test-suite=integration on the binary directly).
  args : ['--duration=true', '--test-suite-exclude=integration'],
  timeout : 60,
  workdir : cwd_for_tests,
)

test('integration', unit_test_exe,
  # Long-running headless game tests: save/load round-trips, determinism
  # cross-validation across seeds, multi-turn AI sanity, etc.  Tagged
  # with TEST_SUITE_BEGIN("integration") at the source.  Expect ~73s on
  # current hardware.  Run on a slower cadence than the fast loop —
  # typically every few commits / pre-push, not pre-commit.
  args : ['--duration=true', '--test-suite=integration'],
  timeout : 180,
  workdir : cwd_for_tests,
)

# e2e smoke test: launches the real game binary and drives it via Unix socket
find_program('python3', required : true)
test('smoke',
  find_program('test/smoke_test.py'),
  args : [join_paths(meson.project_build_root(), 'ctp2')],
  timeout : 120,
  is_parallel : false,
  priority : -1,
  workdir : cwd_for_tests,
)
