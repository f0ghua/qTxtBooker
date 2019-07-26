## Debug script example (script.bat)

set QT_LOGGING_RULES=app.ngidevice=false;app.*=false
set QT_MESSAGE_PATTERN=%%{time yyyy-dd-MM HH:mm:ss.zzz} %%{type} %%{category} %%{message}
rem set QT_MESSAGE_PATTERN=[%%{time yyyyMMdd h:mm:ss.zzz} %%{if-debug}D%%{endif}%%{if-info}I%%{endif}%%{if-warning}W%%{endif}%%{if-critical}C%%{endif}%%{if-fatal}F%%{endif}] %%{file}:%%{line} - %%{message}
start fuseTester.exe