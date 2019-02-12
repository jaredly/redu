#include <fluid_shared.h>

void redu_execCommand(value cwd, value command, value args, value cb_v) {
  CAMLparam4(cwd, command, args, cb_v);
  int callbackId = Int_val(cb_v);

  NSMutableArray<NSString*>* arguments = [NSMutableArray array];
  int count = Wosize_val(args);
  for (int i=0; i<count; i++) {
    [arguments addObject:NSString_val(Field(args, i))];
  }

  NSPipe *stdout = [NSPipe pipe];
  NSPipe *stderr = [NSPipe pipe];

  NSTask *task = [[NSTask alloc] init];
  task.arguments = arguments;
  task.executableURL = [NSURL fileURLWithPath:NSString_val(command)];
  task.currentDirectoryURL = [NSURL fileURLWithPath:NSString_val(cwd)];
  task.standardOutput = stdout;
  task.standardError = stderr;
  task.terminationHandler = ^(NSTask *ignored){
    dispatch_async(dispatch_get_main_queue(), ^{
      CAMLparam0();
      CAMLlocal1(tuple_v);

      static value * closure_f = NULL;
      if (closure_f == NULL) {
          /* First time around, look up by name */
          closure_f = caml_named_value("redu_execCommand_cb");
      }

      tuple_v = caml_alloc_tuple(3);
      Store_field(tuple_v, 0, caml_copy_string([[[NSString alloc] initWithData:[stdout.fileHandleForReading readDataToEndOfFile] encoding:NSUTF8StringEncoding] UTF8String]));
      Store_field(tuple_v, 1, caml_copy_string([[[NSString alloc] initWithData:[stderr.fileHandleForReading readDataToEndOfFile] encoding:NSUTF8StringEncoding] UTF8String]));
      Store_field(tuple_v, 2, Val_int(task.terminationStatus));

      caml_callback2(*closure_f, Val_int(callbackId), tuple_v);
      CAMLreturn0;
    });
  };
  [task launch];

  CAMLreturn0;
}

void redu_openDirectory(value current, value cb_v) {
  CAMLparam2(current, cb_v);

  int callbackId = Int_val(cb_v);
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.canChooseFiles = NO;
  panel.canChooseDirectories = YES;
  panel.allowsMultipleSelection = NO;
  panel.directoryURL = [NSURL fileURLWithPath:NSString_val(current)];
  [panel beginWithCompletionHandler:^(NSModalResponse response) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callString(callbackId, [[panel URLs][0] fileSystemRepresentation]);
    });
  }];

  CAMLreturn0;
}