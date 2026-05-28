# Dogeza Sliding - Lecture06 Modulization Based

This project is based on the Lecture06-Modulization structure.
The original GameLoop / GameObject / Component structure is kept, and new components were added for the game rules.

## Controls

- Space: start / approach / prone slide
- R: restart with a new random map
- ESC: quit

## Implemented Features

- Component-based game rules
- FSM game flow
- 3 random maps
  - MAP A: immediate stop
  - MAP B: 1.5 sec inertia
  - MAP C: 3.0 sec inertia
- Receiver NPC target
- Hidden judgement lines
- Background-anchored receiver position
- Receiver proximity glow for visual distance feedback
- Bottom distance meter for close/far feedback
- Follow camera using player world Z depth
- School background zoom around the vanishing point
- Texture sprites using WIC PNG loading
- Bitmap font HUD using a texture atlas

## Scoring Rule

The receiver is the collision boundary. The player must stop in front of the receiver.

- If playerZ >= receiverZ: FAIL
- If 0 < receiverZ - playerZ <= 25: PERFECT
- If 25 < receiverZ - playerZ <= 60: GREAT
- If 60 < receiverZ - playerZ <= 120: GOOD
- Otherwise: FAIL

This means the score is higher when the player stops as far forward as possible without hitting or passing the receiver.

## New Rendering Files

- Texture.hpp
- texture.hlsl
- assets/player_run.png
- assets/player_prone.png
- assets/receiver.png
- assets/font_atlas.png
- assets/kangwon_bg.png

## Main Added Components

- MapRandomizerComponent
- ProneSlideComponent
- DepthTargetComponent
- FollowCameraComponent
- BackgroundZoomComponent
- GameStateMachineComponent
- JudgeComponent
- ScoreComponent
- ReceiverReactionComponent
- ReceiverProximityCueComponent
- DistanceMeterComponent
- BitmapFontHudComponent

## Build

Open `DogezaSliding.sln` in Visual Studio 2022 and build Debug x64.
The project is set to use `/utf-8`.


- Added `assets/receiver_halo.png` as the receiver proximity halo effect.


- Added `CameraShakeComponent`: as the player approaches the receiver, camera shake gradually increases until the player stops.


- Added result word-art textures (`result_perfect.png`, `result_great.png`, `result_good.png`, `result_fail.png`) and a `ResultImageOverlayComponent`.
- Added `ResultFlashComponent` and improved bitmap HUD layout with clearer title / ready / result prompts.


- Added final polish components:
  - `MapFilterComponent` for map-specific color grading.
  - `ResultCameraImpactComponent` for result-impact camera feedback.
  - `ReceiverResultAuraComponent` for receiver-side success/fail feedback.
  - `ResultHaloBurstComponent` for success result burst effects.
  - `FailureSlashComponent` for fail-specific red slash feedback.


- Added post-processing bloom: the scene is rendered to an off-screen render target, copied back to the swap chain, then bright pixels are additively blurred over the final image. Files: `postcopy.hlsl`, `bloom.hlsl`, `GraphicsContext.hpp`, `GameLoop.hpp`.


Bloom brightness preservation fix:
- Base scene copy now uses no blend state instead of alpha blending.
- postcopy.hlsl forces output alpha to 1.0.
- Bloom is added on top of the original scene only.
- Bloom defaults are toned down: threshold 0.72, intensity 0.38, radius 1.75.

- Removed receiver-side translucent color quad (`ReceiverResultAura`) to avoid cheap-looking rectangular overlay. Receiver feedback now relies on halo textures, result image, bloom, and camera impact.

- Added a proper title scene with `assets/title_card.png`, a cinematic dark backdrop, and a large title image that waits for Space.

## Score / Best Score Rule

- PERFECT / GREAT / GOOD add points to the current score.
- The best score is updated whenever the current score exceeds it.
- FAIL resets only the current score to 0.
- The best score remains on screen after FAIL.
- The title image was replaced with a Korean light-novel style logo asset.

- BEST visibility fix: BuildKey now includes BEST and the score panel is moved left so the number remains visible.

- Removed stage-specific background color filter. The school background now keeps its original color across all maps.

- Added per-map background images: MAP A uses day, MAP B uses sunset, and MAP C uses night. The background changes by texture, not by color filter.

- Design cleanup:
  - Removed unused `floorColor`, `wallColor`, and `lineColor` from `MapRule`.
  - Removed `ProjectedLineComponent` because judgement lines are hidden in the final game.
  - Removed `ReceiverReactionComponent` because result feedback is now handled by result image, halo burst, failure slash, camera impact, and bloom effects.
