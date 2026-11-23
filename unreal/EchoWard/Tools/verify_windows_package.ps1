[CmdletBinding()]
param([ValidateSet('Smoke','Physics','Capture','Agent','Menu','Archive','Saves','About','Training','Settings','Difficulty','Pause','HUD','Field','Book','World','WorldCapture','Validate')][string]$Mode='Smoke')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskPackage=Join-Path $taskRoot 'Saved\PackagedWindows'
$taskExe=Join-Path $taskPackage 'Windows\EchoWard\Binaries\Win64\EchoWard.exe'
if(!(Test-Path -LiteralPath $taskExe)){throw 'Windows package has not been built.'}
$taskExeStamp=(Get-Item -LiteralPath $taskExe).LastWriteTimeUtc
$taskContentFiles=@(Get-ChildItem -LiteralPath (Join-Path $taskPackage 'Windows\EchoWard\Content\Paks') -File | Where-Object {$_.Extension -in @('.pak','.ucas','.utoc')} | Sort-Object Name)
if(!$taskContentFiles.Count){throw 'Packaged content archives are missing.'}
$taskBuildHashes=@((Get-FileHash -LiteralPath $taskExe -Algorithm SHA256).Hash)+@($taskContentFiles | ForEach-Object {(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash})
$taskBookManifest=Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/book_map.json'
if(Test-Path -LiteralPath $taskBookManifest){$taskBuildHashes+=@((Get-FileHash -LiteralPath $taskBookManifest -Algorithm SHA256).Hash)}
$taskHospitalManifest=Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/hospital_world.json'
if(Test-Path -LiteralPath $taskHospitalManifest){$taskBuildHashes+=@((Get-FileHash -LiteralPath $taskHospitalManifest -Algorithm SHA256).Hash)}
$taskHospitalData=Get-Content -Raw -LiteralPath $taskHospitalManifest | ConvertFrom-Json
$taskExpectedRooms=(@($taskHospitalData.locations | ForEach-Object {$_.rooms})).Count
$taskExpectedLocations=@($taskHospitalData.locations).Count
foreach($taskCustomName in @('echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json','echo_arrival_v29.json','echo_realism_v30.json','echo_cars_v31.json','echo_car_quality_v32.json','echo_scenario_v35.json','hospital_assets_v37.json','hospital_environment_v37.json')){
    $taskCustom=Join-Path $taskPackage ('Windows/EchoWard/Content/CampaignData/'+$taskCustomName)
    if(Test-Path -LiteralPath $taskCustom){$taskBuildHashes+=@((Get-FileHash -LiteralPath $taskCustom -Algorithm SHA256).Hash)}
}
$taskBuildIdentity=$taskBuildHashes -join ':'
if($Mode -ne 'Validate'){
$taskUser=Join-Path $taskRoot ('Saved\PackageTests\'+$Mode)
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')))
if($Mode -notin @('World','WorldCapture')){$taskArgs+='-WardLegacyCampaign'}
if($Mode -eq 'World'){$taskArgs+=@('-nullrhi','-WardBookWorldCheck','-benchmark','-deterministic','-fps=60')}
elseif($Mode -eq 'WorldCapture'){$taskArgs+=@('-WardBookWorldCheck','-WardBookWorldPictures','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Smoke'){$taskArgs+=@('-nullrhi','-WardCampaignSmoke')}
elseif($Mode -eq 'Physics'){$taskArgs+=@('-nullrhi','-WardPhysicsCheck','-benchmark','-deterministic','-fps=60')}
elseif($Mode -eq 'Menu'){$taskArgs+=@('-WardMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Archive'){$taskArgs+=@('-WardMenuCheck','-WardArchiveCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Book'){$taskArgs+=@('-WardMenuCheck','-WardBookMapCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Field'){$taskArgs+=@('-WardMenuCheck','-WardFieldMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'HUD'){$taskArgs+=@('-WardMenuCheck','-WardHUDCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Pause'){$taskArgs+=@('-WardMenuCheck','-WardPauseMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Difficulty'){$taskArgs+=@('-WardMenuCheck','-WardDifficultyMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Settings'){$taskArgs+=@('-WardMenuCheck','-WardSettingsMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Training'){$taskArgs+=@('-WardMenuCheck','-WardTrainingMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'About'){$taskArgs+=@('-WardMenuCheck','-WardAboutMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Saves'){$taskArgs+=@('-WardMenuCheck','-WardSaveMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
elseif($Mode -eq 'Agent'){$taskArgs+=@('-nullrhi','-WardCampaignAgent','-benchmark','-deterministic','-fps=60')}
else{$taskArgs+=@('-WardCampaignCapture','-RenderOffscreen','-windowed','-ForceRes','-ResX=1280','-ResY=720')}
$taskStarted=Get-Date
$taskQuotedArgs=($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' '
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskQuotedArgs -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'package_test_console.log') -RedirectStandardError (Join-Path $taskSaved 'package_test_stderr.log')
if($taskProcess.ExitCode -ne 0){throw "Packaged executable failed with code $($taskProcess.ExitCode)."}
if($Mode -in @('World','WorldCapture')){
    $taskResult=Join-Path $taskSaved 'Prototype/book_world_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh 3D world result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 38 -or $taskData.rooms -ne $taskExpectedRooms -or $taskData.locations -ne $taskExpectedLocations -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Complete 3D world checks failed.'}
    if($Mode -eq 'WorldCapture'){
        foreach($taskName in @($taskHospitalData.locations | ForEach-Object {$_.id})){
            $taskImage=Join-Path $taskSaved ('Prototype/BookWorldCaptures/world_'+$taskName+'.png')
            if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh 3D world capture '+$taskName)}
        }
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy|Non-Nanite Marking Job Queue overflow|Virtual Shadow Map.*overflow|surface cache atlas oversubscribed'){throw '3D world runtime failure.'}
}elseif($Mode -eq 'Smoke'){
    $taskResult=Join-Path $taskSaved 'Prototype\campaign_smoke.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'Packaged game produced no fresh campaign result. Check default map and CampaignData staging.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 54 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Packaged smoke checks failed.'}
}elseif($Mode -eq 'Book'){
    $taskResult=Join-Path $taskSaved 'Prototype/book_map_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh full book map result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 27 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Full book map checks failed.'}
    foreach($taskName in @('book_all_1080','book_1F_1080','book_B1_1080','book_DG_1080','book_2F_1080','book_IF_1080','book_PS_1080','book_3F_1080','book_RH_1080','book_HS_1080','book_SV_1080','book_ST_1080','book_RF_1080','book_B2_1080','book_en_1080','book_kk_1080','book_720')){
        $taskImage=Join-Path $taskSaved ('Prototype/BookMapCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh book map image '+$taskName)}
    }
    $taskBook=Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/book_map.json'
    if(!(Test-Path -LiteralPath $taskBook)){throw 'Full book map data missing from Windows package.'}
    if((Get-FileHash -LiteralPath $taskBook).Hash -ne (Get-FileHash -LiteralPath (Join-Path $taskRoot 'Content/CampaignData/book_map.json')).Hash){throw 'Staged book map differs from source.'}
}elseif($Mode -eq 'Field'){
    $taskResult=Join-Path $taskSaved 'Prototype/field_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh field menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 22 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Field menu checks failed.'}
    foreach($taskName in @('inventory_1080','documents_empty_1080','documents_1080','map_locked_1080','map_ward_1080','map_basement_1080','map_archive_1080','inventory_hover_1080','inventory_en_1080','inventory_kk_1080','inventory_720')){
        $taskImage=Join-Path $taskSaved ('Prototype/FieldCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh field image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Field menu runtime failure.'}
}elseif($Mode -eq 'Physics'){
    $taskResult=Join-Path $taskSaved 'Prototype\physics_assessment.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh packaged physics assessment.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.checks.Count -lt 8 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Packaged physics checks failed.'}
 }elseif($Mode -eq 'Menu'){
    $taskResult=Join-Path $taskSaved 'Prototype/menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh packaged menu report.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 56){throw 'Packaged menu checks failed.'}
    foreach($taskName in @('menu_1080','corridor_1080','game_1080','gallery_1080','menu_kk_1080','menu_en_1080','menu_hover_1080','menu_card_hover_1080','door_open_1080','scare_1080','nurse_1080','zombie_head_1080','ball_roll_1080','ball_wait_1080','hand_reach_1080','entrance_idle_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/MenuCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh menu capture '+$taskName)}
    }
    $taskFallbacks=Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'
    if($taskFallbacks){throw 'Runtime menu material fallback detected.'}
}elseif($Mode -eq 'Archive'){
    $taskResult=Join-Path $taskSaved 'Prototype/archive_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh archive menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 23 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Archive menu checks failed.'}
    foreach($taskName in @('game_glass_1080','game_glass_focus_1080','archive_corridor_1080','archive_second_room_1080','archive_third_room_1080','archive_zombie_1080','settings_hover_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/ArchiveCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh archive image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Archive menu runtime material failure.'}
}elseif($Mode -eq 'Saves'){
    $taskResult=Join-Path $taskSaved 'Prototype/save_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh saves menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 21 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Save menu checks failed.'}
    foreach($taskName in @('saves_1080','saves_hover_1080','save_room_1080','saves_ingame_1080','saves_kk_1080','saves_en_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/SaveMenuCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh save menu image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Save menu runtime material failure.'}
 }elseif($Mode -eq 'HUD'){
    $taskResult=Join-Path $taskSaved 'Prototype/hud_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh HUD result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 18 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'HUD checks failed.'}
    foreach($taskName in @('hud_1080','hud_interaction_1080','hud_shotgun_1080','hud_kk_1080','hud_en_1080','hud_720','hud_1200')){
        $taskImage=Join-Path $taskSaved ('Prototype/HUDCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh HUD image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material'){throw 'HUD runtime material failure.'}
 }elseif($Mode -eq 'Pause'){
    $taskResult=Join-Path $taskSaved 'Prototype/pause_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh pause menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 28 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Pause menu checks failed.'}
    foreach($taskName in @('pause_1080','pause_hover_1080','pause_nav_hover_1080','pause_room_1080','pause_kk_1080','pause_en_1080','pause_admin_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/PauseCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh pause image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Pause runtime material failure.'}
 }elseif($Mode -eq 'Difficulty'){
    $taskResult=Join-Path $taskSaved 'Prototype/difficulty_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh difficulty menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.count -lt 22 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Difficulty menu checks failed.'}
    foreach($taskName in @('difficulty_1080','difficulty_hover_1080','difficulty_room_1080','difficulty_kk_1080','difficulty_en_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/DifficultyCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh difficulty image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Difficulty runtime material failure.'}
 }elseif($Mode -eq 'Settings'){
    $taskResult=Join-Path $taskSaved 'Prototype/settings_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh settings menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    # Assert the named checks implemented by WardSettingsMenuAssessment, not a stale total.
    $taskRequiredSettings=@('settings_has_quiet_live_background','camera_and_light_animate_while_paused','no_menu_monsters_visible','four_tabs_and_real_sliders','no_yellow_gameplay_hint','focus_does_not_turn_choice_red','choice_red_on_hover','choice_neutral_after_pointer_leaves','resolution_arrows_round_trip','performance_preset_really_uses_75_percent_tsr','anti_aliasing_choice_really_disables_tsr','slider_values_apply_and_clamp','grain_slider_can_disable_grain','motion_blur_switch_changes_real_option','options_saved_to_disk','audio_controls_present','mouse_settings_and_control_guide_retained','language_hud_and_admin_retained','wrong_admin_password_has_visible_feedback','kazakh_copy_visible','english_copy_visible','back_restores_main_menu','settings_back_restores_live_pause_without_moving_hero','reset_restores_defaults')
    $taskActualSettings=@($taskData.checks | ForEach-Object {$_.name})
    $taskSettingsMissing=@($taskRequiredSettings | Where-Object {$_ -notin $taskActualSettings})
    if(!$taskData.success -or $taskData.count -ne $taskData.checks.Count -or $taskSettingsMissing.Count -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Settings menu checks failed.'}
    foreach($taskName in @('settings_graphics_1080','settings_audio_1080','settings_controls_1080','settings_game_1080','settings_hover_1080','settings_room_1080','settings_kk_1080','settings_en_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/SettingsCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh settings menu image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Settings menu runtime material failure.'}
 }elseif($Mode -eq 'Training'){
    $taskResult=Join-Path $taskSaved 'Prototype/training_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh training menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or  $taskData.count -lt 25 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'Training menu checks failed.'}
    foreach($taskName in @('training_01_1080','training_02_1080','training_03_1080','training_04_1080','training_05_1080','training_06_1080','training_07_1080','training_08_1080','training_hover_1080','training_room_1080','training_kk_1080','training_en_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/TrainingCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh training menu image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Training menu runtime material failure.'}
 }elseif($Mode -eq 'About'){
    $taskResult=Join-Path $taskSaved 'Prototype/about_menu_check.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh about menu result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or  $taskData.count -lt 18 -or @($taskData.checks | Where-Object {!$_.passed}).Count){throw 'About menu checks failed.'}
    foreach($taskName in @('about_01_1080','about_02_1080','about_03_1080','about_04_1080','about_hover_1080','about_room_1080','about_kk_1080','about_en_1080')){
        $taskImage=Join-Path $taskSaved ('Prototype/AboutCaptures/'+$taskName+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh about menu image '+$taskName)}
    }
    if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'About menu runtime material failure.'}
}elseif($Mode -eq 'Agent'){
    $taskResult=Join-Path $taskSaved 'Prototype\campaign_agent.json'
    if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'Packaged physical route produced no fresh result.'}
    $taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if(!$taskData.success -or $taskData.resumed){throw 'Packaged physical route did not win from a fresh game.'}
}else{
    foreach($taskIndex in 0..27){
        $taskImage=Join-Path $taskSaved ('Prototype\CampaignCaptures\view_'+$taskIndex+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw "Missing fresh packaged screenshot $taskIndex."}
    }
    foreach($taskIndex in 18..21){
        $taskPose=Get-Content -Raw -LiteralPath (Join-Path $taskSaved ('Prototype\CampaignCaptures\pose_'+$taskIndex+'.json')) | ConvertFrom-Json
        if($taskPose.right_grip_error_cm -gt 3 -or $taskPose.right_palm_down_alignment -lt .8){throw "Invalid pistol palm orientation in close-up $taskIndex."}
        $taskTriggerFinger=$taskPose.right_finger_joints_weapon_cm.'Bip01 R Finger12'
        if(!$taskTriggerFinger -or $taskTriggerFinger[0] -lt 3 -or $taskTriggerFinger[0] -gt 9 -or $taskTriggerFinger[2] -lt -10 -or $taskTriggerFinger[2] -gt -2){throw "Trigger finger misses the trigger-guard area in close-up $taskIndex."}
    }
    foreach($taskIndex in 11..16){
        $taskPose=Get-Content -Raw -LiteralPath (Join-Path $taskSaved ('Prototype\CampaignCaptures\pose_'+$taskIndex+'.json')) | ConvertFrom-Json
        if($null -eq $taskPose.right_grip_error_cm -or [double]::IsNaN([double]$taskPose.right_grip_error_cm) -or [double]::IsInfinity([double]$taskPose.right_grip_error_cm) -or $taskPose.right_grip_error_cm -gt 3){throw "Right hand is detached from grip in pose $taskIndex."}
        if($taskIndex -ge 12 -and ($null -eq $taskPose.left_grip_error_cm -or [double]::IsNaN([double]$taskPose.left_grip_error_cm) -or [double]::IsInfinity([double]$taskPose.left_grip_error_cm) -or $taskPose.left_grip_error_cm -gt 3)){throw "Support hand is detached from grip in pose $taskIndex."}
    }
    $taskShader=Get-Content -Raw -LiteralPath (Join-Path $taskSaved 'Prototype\shader_validation.json') | ConvertFrom-Json
    if(!$taskShader.success -or $taskShader.materials_checked -lt 258){throw 'GPU material validation failed; a default material may be in use.'}
    $taskFallbacks=Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'missing usage flag|Invalid material.*Nanite|Default Material will be used|Failed to compile Material'
    if($taskFallbacks){throw 'Runtime material fallback detected; inspect package_test.log.'}
}
$taskStatus=[ordered]@{success=$true;mode=$Mode;checked_at=(Get-Date).ToString('o');executable=$taskExe;executable_timestamp=(Get-Item -LiteralPath $taskExe).LastWriteTimeUtc.ToString('o');build_identity=$taskBuildIdentity;test_saved=$taskSaved}
$taskStatus | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage ('verified_'+$Mode.ToLowerInvariant()+'.json')) -Encoding UTF8
}
$taskSmoke=Join-Path $taskPackage 'verified_smoke.json'
$taskCapture=Join-Path $taskPackage 'verified_capture.json'
if((Test-Path -LiteralPath $taskSmoke) -and (Test-Path -LiteralPath $taskCapture)){
    $taskS=Get-Content -Raw -LiteralPath $taskSmoke | ConvertFrom-Json
    $taskC=Get-Content -Raw -LiteralPath $taskCapture | ConvertFrom-Json
    if($taskS.success -and $taskC.success -and $taskS.build_identity -eq $taskBuildIdentity -and $taskC.build_identity -eq $taskBuildIdentity){
        $taskMarker=[ordered]@{success=$true;checked_at=(Get-Date).ToString('o');executable=$taskExe;executable_timestamp=$taskExeStamp.ToString('o');smoke_checks=54;captured_views=28;gpu_materials_checked=258;physical_route_verified=$false}
        $taskMarker['build_identity']=$taskBuildIdentity
        $taskPhysicsStatus=Join-Path $taskPackage 'verified_physics.json'
        if(Test-Path -LiteralPath $taskPhysicsStatus){$taskP=Get-Content -Raw -LiteralPath $taskPhysicsStatus | ConvertFrom-Json;if($taskP.success -and $taskP.build_identity -eq $taskBuildIdentity){$taskMarker['physics_checks']=8}}
        $taskMenuStatus=Join-Path $taskPackage 'verified_menu.json'
        if(Test-Path -LiteralPath $taskMenuStatus){$taskM=Get-Content -Raw -LiteralPath $taskMenuStatus | ConvertFrom-Json;if($taskM.success -and $taskM.build_identity -eq $taskBuildIdentity){$taskMenuResult=Get-Content -Raw -LiteralPath (Join-Path $taskM.test_saved 'Prototype/menu_check.json') | ConvertFrom-Json;$taskMarker['menu_checks']=[int]$taskMenuResult.count;$taskMarker['menu_resolution']='1920x1080'}}
        $taskHUDStatus=Join-Path $taskPackage 'verified_hud.json'
        if(Test-Path -LiteralPath $taskHUDStatus){$taskHUD=Get-Content -Raw -LiteralPath $taskHUDStatus | ConvertFrom-Json;if($taskHUD.success -and $taskHUD.build_identity -eq $taskBuildIdentity){$taskHUDResult=Get-Content -Raw -LiteralPath (Join-Path $taskHUD.test_saved 'Prototype/hud_check.json') | ConvertFrom-Json;$taskMarker['hud_checks']=[int]$taskHUDResult.count}}
        $taskPauseStatus=Join-Path $taskPackage 'verified_pause.json'
        if(Test-Path -LiteralPath $taskPauseStatus){$taskPause=Get-Content -Raw -LiteralPath $taskPauseStatus | ConvertFrom-Json;if($taskPause.success -and $taskPause.build_identity -eq $taskBuildIdentity){$taskPauseResult=Get-Content -Raw -LiteralPath (Join-Path $taskPause.test_saved 'Prototype/pause_menu_check.json') | ConvertFrom-Json;$taskMarker['pause_menu_checks']=[int]$taskPauseResult.count;$taskMarker['pause_menu_resolution']='1920x1080'}}
        $taskDifficultyStatus=Join-Path $taskPackage 'verified_difficulty.json'
        if(Test-Path -LiteralPath $taskDifficultyStatus){$taskD=Get-Content -Raw -LiteralPath $taskDifficultyStatus | ConvertFrom-Json;if($taskD.success -and $taskD.build_identity -eq $taskBuildIdentity){$taskDResult=Get-Content -Raw -LiteralPath (Join-Path $taskD.test_saved 'Prototype/difficulty_menu_check.json') | ConvertFrom-Json;$taskMarker['difficulty_menu_checks']=[int]$taskDResult.count;$taskMarker['difficulty_menu_resolution']='1920x1080'}}
        $taskSettingsStatus=Join-Path $taskPackage 'verified_settings.json'
        if(Test-Path -LiteralPath $taskSettingsStatus){$taskQ=Get-Content -Raw -LiteralPath $taskSettingsStatus | ConvertFrom-Json;if($taskQ.success -and $taskQ.build_identity -eq $taskBuildIdentity){$taskQResult=Get-Content -Raw -LiteralPath (Join-Path $taskQ.test_saved 'Prototype/settings_menu_check.json') | ConvertFrom-Json;$taskMarker['settings_menu_checks']=[int]$taskQResult.count;$taskMarker['settings_menu_resolution']='1920x1080'}}
        $taskTrainingStatus=Join-Path $taskPackage 'verified_training.json'
        if(Test-Path -LiteralPath $taskTrainingStatus){$taskT=Get-Content -Raw -LiteralPath $taskTrainingStatus | ConvertFrom-Json;if($taskT.success -and $taskT.build_identity -eq $taskBuildIdentity){$taskTResult=Get-Content -Raw -LiteralPath (Join-Path $taskT.test_saved 'Prototype/training_menu_check.json') | ConvertFrom-Json;$taskMarker['training_menu_checks']=[int]$taskTResult.count;$taskMarker['training_menu_resolution']='1920x1080'}}
        $taskAboutStatus=Join-Path $taskPackage 'verified_about.json'
        if(Test-Path -LiteralPath $taskAboutStatus){$taskB=Get-Content -Raw -LiteralPath $taskAboutStatus | ConvertFrom-Json;if($taskB.success -and $taskB.build_identity -eq $taskBuildIdentity){$taskBResult=Get-Content -Raw -LiteralPath (Join-Path $taskB.test_saved 'Prototype/about_menu_check.json') | ConvertFrom-Json;$taskMarker['about_menu_checks']=[int]$taskBResult.count;$taskMarker['about_menu_resolution']='1920x1080'}}
        $taskBookStatus=Join-Path $taskPackage 'verified_book.json'
        if(Test-Path -LiteralPath $taskBookStatus){$taskBookData=Get-Content -Raw -LiteralPath $taskBookStatus | ConvertFrom-Json;if($taskBookData.success -and $taskBookData.build_identity -eq $taskBuildIdentity){$taskBookResult=Get-Content -Raw -LiteralPath (Join-Path $taskBookData.test_saved 'Prototype/book_map_check.json') | ConvertFrom-Json;$taskMarker['book_map_checks']=[int]$taskBookResult.count;$taskMarker['book_map_rooms']=328;$taskMarker['book_map_locations']=13}}
        $taskFieldStatus=Join-Path $taskPackage 'verified_field.json'
        if(Test-Path -LiteralPath $taskFieldStatus){$taskF=Get-Content -Raw -LiteralPath $taskFieldStatus | ConvertFrom-Json;if($taskF.success -and $taskF.build_identity -eq $taskBuildIdentity){$taskFResult=Get-Content -Raw -LiteralPath (Join-Path $taskF.test_saved 'Prototype/field_menu_check.json') | ConvertFrom-Json;$taskMarker['field_menu_checks']=[int]$taskFResult.count}}
        $taskAgentStatus=Join-Path $taskPackage 'verified_agent.json'
        $taskSavesStatus=Join-Path $taskPackage 'verified_saves.json'
        if(Test-Path -LiteralPath $taskSavesStatus){$taskV=Get-Content -Raw -LiteralPath $taskSavesStatus | ConvertFrom-Json;if($taskV.success -and $taskV.build_identity -eq $taskBuildIdentity){$taskVResult=Get-Content -Raw -LiteralPath (Join-Path $taskV.test_saved 'Prototype/save_menu_check.json') | ConvertFrom-Json;$taskMarker['save_menu_checks']=[int]$taskVResult.count;$taskMarker['save_menu_resolution']='1920x1080'}}
        $taskArchiveStatus=Join-Path $taskPackage 'verified_archive.json'
        if(Test-Path -LiteralPath $taskArchiveStatus){$taskR=Get-Content -Raw -LiteralPath $taskArchiveStatus | ConvertFrom-Json;if($taskR.success -and $taskR.build_identity -eq $taskBuildIdentity){$taskResult=Get-Content -Raw -LiteralPath (Join-Path $taskR.test_saved 'Prototype/archive_check.json') | ConvertFrom-Json;$taskMarker['archive_checks']=[int]$taskResult.count;$taskMarker['archive_resolution']='1920x1080';$taskMarker['zombie_room_wait_seconds']=$taskResult.room_wait_real_seconds}}
        if(Test-Path -LiteralPath $taskAgentStatus){$taskA=Get-Content -Raw -LiteralPath $taskAgentStatus | ConvertFrom-Json;if($taskA.success -and $taskA.build_identity -eq $taskBuildIdentity){$taskAgentData=Get-Content -Raw -LiteralPath (Join-Path $taskA.test_saved 'Prototype\campaign_agent.json') | ConvertFrom-Json;$taskMarker.physical_route_verified=[bool]$taskAgentData.success;$taskMarker['agent_seconds']=$taskAgentData.elapsed}}
        if($taskMarker.physics_checks -eq 8){$taskMarker | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'package_verified.json') -Encoding UTF8}
        elseif($Mode -eq 'Validate'){throw 'Current build also requires matching Physics checks.'}
    }
}
$taskWorldStatus=Join-Path $taskPackage 'verified_world.json'
$taskWorldCaptureStatus=Join-Path $taskPackage 'verified_worldcapture.json'
if((Test-Path -LiteralPath $taskWorldStatus) -and (Test-Path -LiteralPath $taskWorldCaptureStatus)){
    $taskWorld=Get-Content -Raw -LiteralPath $taskWorldStatus | ConvertFrom-Json
    $taskWorldCapture=Get-Content -Raw -LiteralPath $taskWorldCaptureStatus | ConvertFrom-Json
    if($taskWorld.success -and $taskWorldCapture.success -and $taskWorld.build_identity -eq $taskBuildIdentity -and $taskWorldCapture.build_identity -eq $taskBuildIdentity){
        $taskWorldResult=Get-Content -Raw -LiteralPath (Join-Path $taskWorld.test_saved 'Prototype/book_world_check.json') | ConvertFrom-Json
        [ordered]@{success=$true;checked_at=(Get-Date).ToString('o');executable=$taskExe;executable_timestamp=$taskExeStamp.ToString('o');build_identity=$taskBuildIdentity;world_id=$taskHospitalData.map_id;world_checks=[int]$taskWorldResult.count;world_rooms=$taskExpectedRooms;world_locations=$taskExpectedLocations;world_captures=$taskExpectedLocations;physical_stair_and_gallery_walk_verified=$true;physical_tunnel_walk_verified=$true} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'package_verified.json') -Encoding UTF8
    }
}
if($Mode -eq 'Validate'){
    if(!(Test-Path -LiteralPath (Join-Path $taskPackage 'package_verified.json'))){throw 'Both packaged World and WorldCapture must pass for the current executable.'}
    $taskValidated=Get-Content -Raw -LiteralPath (Join-Path $taskPackage 'package_verified.json') | ConvertFrom-Json
    if($taskValidated.build_identity -ne $taskBuildIdentity -or $taskValidated.world_id -ne $taskHospitalData.map_id){throw 'Current default 3D world must pass World and WorldCapture.'}
    $taskValidated | ConvertTo-Json
}else{$taskStatus | ConvertTo-Json}

