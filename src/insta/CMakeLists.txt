function(insta_patch_engine)
    set_src(INSTA_ENGINE_INTERFACE GLOB src/insta/engine
      gameserver.h
      server.h
    )

    set_src(INSTA_ENGINE_SHARED GLOB_RECURSE src/insta/engine/shared
      config_variables_insta.h
    )

    set_src(INSTA_INCLUDES GLOB_RECURSE src/insta/includes
      character.h
      chat_commands.h
      engine/gameserver.h
      engine/server.h
      engine/server/server.h
      engine/shared/config_variables.h
      gamecontext.h
      gamecontroller.h
      player.h
      rcon_commands.h
    )

    set(ENGINE_INTERFACE ${ENGINE_INTERFACE} PARENT_SCOPE)
    list(APPEND ENGINE_INTERFACE ${INSTA_ENGINE_INTERFACE})
    set(ENGINE_INTERFACE ${ENGINE_INTERFACE} PARENT_SCOPE)

    set(ENGINE_SHARED ${ENGINE_SHARED} PARENT_SCOPE)
    list(APPEND ENGINE_SHARED ${INSTA_INCLUDES} ${INSTA_ENGINE_SHARED})
    set(ENGINE_SHARED ${ENGINE_SHARED} PARENT_SCOPE)
endfunction()

function(insta_patch_server)
    file(GLOB_RECURSE INSTA_EXTERNAL_GAMEMODES "src/external_gamemodes/*.cpp" "src/external_gamemodes/*.h")

    set_src(INSTA_SERVER GLOB_RECURSE src/insta/server
      antibob.cpp
      antibob.h
      chat_commands.cpp
      chat_commands.h
      column_template.h
      ddnet_db_utils/ddnet_db_utils.cpp
      ddnet_db_utils/ddnet_db_utils.h
      dead_spec_controller.cpp
      dead_spec_controller.h
      entities/character.cpp
      entities/character.h
      entities/ddnet_pvp/vanilla_pickup.cpp
      entities/ddnet_pvp/vanilla_pickup.h
      entities/ddnet_pvp/vanilla_projectile.cpp
      entities/ddnet_pvp/vanilla_projectile.h
      entities/flag.cpp
      entities/flag.h
      entities/foot/foot_pickup.cpp
      entities/foot/foot_pickup.h
      entities/foot/foot_projectile.cpp
      entities/foot/foot_projectile.h
      entities/text/laser.cpp
      entities/text/laser.h
      entities/text/projectile.cpp
      entities/text/projectile.h
      entities/text/text.cpp
      entities/text/text.h
      enums.cpp
      enums.h
      extra_columns.h
      gamecontext.cpp
      gamecontext.h
      gamecontext/bangcommands.cpp
      gamecontroller.cpp
      gamecontroller.h
      gamemodes/ball/base_foot.cpp
      gamemodes/ball/base_foot.h
      gamemodes/ball/foot/foot.cpp
      gamemodes/ball/foot/foot.h
      gamemodes/ball/foot/sql_columns.h
      gamemodes/base_pvp/base_pvp.cpp
      gamemodes/base_pvp/base_pvp.h
      gamemodes/base_pvp/chat.cpp
      gamemodes/ddrace/block/block.cpp
      gamemodes/ddrace/block/block.h
      gamemodes/insta_core/insta_core.cpp
      gamemodes/insta_core/insta_core.h
      gamemodes/instagib/base_fng.cpp
      gamemodes/instagib/base_fng.h
      gamemodes/instagib/base_instagib.cpp
      gamemodes/instagib/base_instagib.h
      gamemodes/instagib/bolofng/bolofng.cpp
      gamemodes/instagib/bolofng/bolofng.h
      gamemodes/instagib/bolofng/sql_columns.h
      gamemodes/instagib/boomfng/boomfng.cpp
      gamemodes/instagib/boomfng/boomfng.h
      gamemodes/instagib/boomfng/sql_columns.h
      gamemodes/instagib/ctf.cpp
      gamemodes/instagib/ctf.h
      gamemodes/instagib/dm.cpp
      gamemodes/instagib/dm.h
      gamemodes/instagib/fng/fng.cpp
      gamemodes/instagib/fng/fng.h
      gamemodes/instagib/fng/sql_columns.h
      gamemodes/instagib/gctf/gctf.cpp
      gamemodes/instagib/gctf/gctf.h
      gamemodes/instagib/gctf/sql_columns.h
      gamemodes/instagib/gdm/gdm.cpp
      gamemodes/instagib/gdm/gdm.h
      gamemodes/instagib/gtdm/gtdm.cpp
      gamemodes/instagib/gtdm/gtdm.h
      gamemodes/instagib/ictf/ictf.cpp
      gamemodes/instagib/ictf/ictf.h
      gamemodes/instagib/ictf/sql_columns.h
      gamemodes/instagib/idm/idm.cpp
      gamemodes/instagib/idm/idm.h
      gamemodes/instagib/idm/sql_columns.h
      gamemodes/instagib/itdm/itdm.cpp
      gamemodes/instagib/itdm/itdm.h
      gamemodes/instagib/solofng/solofng.cpp
      gamemodes/instagib/solofng/solofng.h
      gamemodes/instagib/solofng/sql_columns.h
      gamemodes/instagib/tdm.cpp
      gamemodes/instagib/tdm.h
      gamemodes/instagib/team_fng.cpp
      gamemodes/instagib/team_fng.h
      gamemodes/instagib/zcatch/colors.cpp
      gamemodes/instagib/zcatch/sql_columns.h
      gamemodes/instagib/zcatch/track_caught_time.cpp
      gamemodes/instagib/zcatch/zcatch.cpp
      gamemodes/instagib/zcatch/zcatch.h
      gamemodes/vanilla/base_ctf.cpp
      gamemodes/vanilla/base_ctf.h
      gamemodes/vanilla/base_vanilla.cpp
      gamemodes/vanilla/base_vanilla.h
      gamemodes/vanilla/bomb/bomb.cpp
      gamemodes/vanilla/bomb/bomb.h
      gamemodes/vanilla/bomb/sql_columns.h
      gamemodes/vanilla/ctf/ctf.cpp
      gamemodes/vanilla/ctf/ctf.h
      gamemodes/vanilla/ctf/sql_columns.h
      gamemodes/vanilla/dm/dm.cpp
      gamemodes/vanilla/dm/dm.h
      gamemodes/vanilla/dm/sql_columns.h
      gamemodes/vanilla/fly/fly.cpp
      gamemodes/vanilla/fly/fly.h
      gamemodes/vanilla/tsmash/tsmash.cpp
      gamemodes/vanilla/tsmash/tsmash.h
      ip_storage.cpp
      ip_storage.h
      persistent_client_data.h
      persistent_data.h
      player.cpp
      player.h
      protocol.h
      rcon_commands.cpp
      rcon_commands.h
      rcon_configs.cpp
      round_stats.cpp
      round_stats_ascii.cpp
      round_stats_csv.cpp
      round_stats_one_player.cpp
      round_stats_player.h
      skin_info_manager.cpp
      skin_info_manager.h
      sql_columns_all.h
      sql_stats.cpp
      sql_stats.h
      sql_stats_player.h
      strhelpers.cpp
      strhelpers.h
      structs.h
      version.h
    )

    set_src(INSTA_ENGINE_SERVER GLOB_RECURSE src/insta/engine/server
      server.cpp
      server.h
    )

    set(GAME_SERVER ${GAME_SERVER} PARENT_SCOPE)
    list(APPEND GAME_SERVER ${INSTA_EXTERNAL_GAMEMODES} ${INSTA_SERVER})
    set(GAME_SERVER ${GAME_SERVER} PARENT_SCOPE)

    set(ENGINE_SERVER_WITHOUT_MAIN ${ENGINE_SERVER_WITHOUT_MAIN} PARENT_SCOPE)
    list(APPEND ENGINE_SERVER_WITHOUT_MAIN ${INSTA_ENGINE_SERVER})
    set(ENGINE_SERVER_WITHOUT_MAIN ${ENGINE_SERVER_WITHOUT_MAIN} PARENT_SCOPE)
endfunction()
