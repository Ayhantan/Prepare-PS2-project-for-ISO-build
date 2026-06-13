## PS2SDK / gsKit Compatibility Rules

This project uses the Docker image:

ps2dev/ps2dev:latest

Known compatibility constraints:
- Do not use GS_MODE_AUTO.
- Use GS_MODE_NTSC unless explicitly changed.
- Do not use gsKit_vsync(); it is not available in this SDK version.
- Use gsKit_queue_exec(gsGlobal) and gsKit_sync_flip(gsGlobal) in the render loop.
- Always verify API names against the headers inside:
  /usr/local/ps2dev/gsKit/include
  /usr/local/ps2dev/ps2sdk/ee/include
- Every code change must compile with:
  make clean && make