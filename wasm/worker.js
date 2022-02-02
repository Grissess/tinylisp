var output = null;
var stdin = "", stdout = "", yielded = false;
var mod, inst;
var loop = null;
var MoreData = {};

function crankNL() {
	if(stdin.indexOf("\n") != -1) crank();
}

function crank(once) {
	if(inst == null || loop == null) return;
	try {
		while(stdin.length > 0 || once) {
			once = false;
			if(yielded) {
				if(stdin.length == 0) break;
				var val = inst.exports.tl_new_int(interp, stdin.charCodeAt(0));
				stdin = stdin.substr(1);
				inst.exports.tl_wasm_values_push(interp, val);
				yielded = false;
			}
			var res = loop.next();
			if(res.done) {
				sysprint("Main loop exited.\n");
				loop = null;
				return;
			}
			if(res.value === MoreData) {
				yielded = true;
				continue;
			}
			throw new Error("Not sure how to handle suspension of value " + res.value);
		}
	} catch(e) {
		sysprint("A fatal error occurred: " + e + "\n");
		throw e;
	}
}

function eprint(x) {
	flush();
	postMessage({type: "stderr", text: x});
}

function sysprint(x) {
	flush();
	postMessage({type: "system", text: x});
}

function print(x) {
	stdout += x;
	if(stdout.indexOf("\n") != -1) {
		flush();
	}
}

function flush() {
	if(stdout.length > 0) {
		postMessage({type: "stdout", text: stdout});
		stdout = "";
	}
}

onmessage = function(e) {
	switch(e.data.type) {
		case "keydown":
			var c = null;
			if(e.data.key.length == 1) {
				c = e.data.key;
			}
			if(e.data.key == "Enter") {
				c = "\n";
			}
			if(e.data.key == "Backspace" && stdin.length > 0) {
				stdin = stdin.substr(0, stdin.length - 1);
				postMessage({type: "stdin", text: "\b"});
			}
			if(c != null) {
				stdin += c;
				postMessage({type: "stdin", text: c});
				crankNL();
			}
			break;
		case "paste":
			if(e.data.text.length > 0) {
				stdin += e.data.text;
				postMessage({type: "stdin", text: e.data.text});
				crankNL();
			}
			break;
		default:
			console.log("Unknown message:", e.data);
			break;
	}
}

var STDIN = 0, STDOUT = 1, STDERR = 2, EOF = -1, PAGE = 65536, PTR = 4;
var watermark = null, interp = null, expr = null, pm_name = null, _main_k_ref = null, main_then = null;
var running = true;
function PGOF(x) { return Math.floor(x / PAGE); }
var PM_NAME = "PRIME_MOVER";
var ENC = new TextEncoder(), DEC = new TextDecoder();

function init() {
	// Set aside a tl_interp on the heap
	watermark = inst.exports['__heap_base'].value
	// As of this writing, sizeof(tl_interp) == 168
	// TODO: find a way to export this information
	interp = watermark; watermark += 256;
	expr = watermark; watermark += PTR;
	pm_name = watermark;
	var mem = inst.exports['memory'];
	var u8a = new Uint8Array(mem, pm_name);
	var res = ENC.encodeInto(PM_NAME, u8a);
	u8a[res.written] = 0;
	watermark += res.written + 1;
	sysprint("Interp at " + interp + ", expr at " + expr + ", pm_name at " + pm_name + " len " + res.written + " value " + DEC.decode(new Uint8Array(mem, pm_name, res.written)) + "\n");
	// Align the watermark
	if(watermark&7 != 0) watermark += (8 - 7&watermark);
	var pw = PGOF(watermark), pgs = PGOF(mem.buffer.byteLength);
	if(pw > pgs) { mem.grow(pw - pgs); }
	// Find the prime mover
	var _main_k = inst.exports._main_read_k;
	var tbl = inst.exports.__indirect_function_table;
	for(var i = 0; i < tbl.length; i++) {
		if(tbl.get(i) === _main_k) {
			_main_k_ref = i;
			break;
		}
	}
	if(_main_k_ref == null) {
		print("Couldn't find prime mover! Refusing to start.");
		return;
	}
	sysprint("Prime mover: " + _main_k_ref + "\n");
	// The rest of this code mirrors tl's ordinary main()
	inst.exports.tl_interp_init(interp);
	loop = mainloop();
	crank(true);
}

function* mainloop() {
	var error;
	while(running) {
		inst.exports.tl_wasm_clear_state(interp);
		inst.exports.tl_gc(interp);  // Take advantage of the cleaner roots
		eprint("> ");
		flush();
		if(main_then == null) {
			main_then = inst.exports.tl_new_then(interp, _main_k_ref, 0, pm_name);
			inst.exports.tl_wasm_make_permanent(main_then);
		}
		inst.exports.tl_push_apply(interp, 1, main_then, inst.exports.tl_wasm_get_env(interp));
		inst.exports.tl_read(interp);
		while(true) {
			var res = inst.exports.tl_apply_next(interp)
			if(res == 0) break;
			switch(res) {
				case 1: break;
				case 2:
					yield MoreData;
					break;
				default:
					console.log("Unkown result:", res);
					break;
			}
		}
		error = inst.exports.tl_wasm_get_error(interp);
		if(error != 0) {
			eprint("Error: ");
			inst.exports.tl_print(interp, error);
			eprint("\n");
		}
	}
	sysprint("Program exited.\n");
}

var imports = {
	tl: {
		fflush: function(fd) {
			flush();
		},
		fputc: function(fd, c) {
			print(String.fromCharCode(c));
		},
		fgetc: function(fd) {
			throw new Error("Should never be called");
		},
		halt: function(code) {
			sysprint("\nProcess exited with code " + code);
			running = false;
			// This isn't supposed to return, but the best we can
			// do is promise not to call any other APIs
		},
		new_heap: function(min, whereptr, szptr) {
			sysprint("new_heap @" + whereptr + " sz@" + szptr + "\n");
			var amt = Math.max(min, 1024*PAGE);
			if(watermark == null) {
				watermark = inst.exports['__heap_base'].value;
			}
			var where = watermark;
			watermark += amt;
			var mem = inst.exports['memory'];
			var pgs = PGOF(mem.buffer.byteLength);
			var maxpg = PGOF(watermark + PAGE - 1);
			if(maxpg > pgs) {
				mem.grow(maxpg - pgs);
				sysprint("memory grown to " + mem.buffer.byteLength + " bytes\n");
			}
			var dv = new DataView(mem.buffer);
			dv.setInt32(whereptr, where, true);
			dv.setInt32(szptr, amt, true);
			sysprint("alloc'd " + dv.getInt32(szptr, true) + " bytes at " + dv.getInt32(whereptr, true) + "\n");
		},
		release_heap: function(where, sz) {},  // Don't care
	},
};

// Work around issues with the MIME type provided by simple servers
fetch("../tl.wasm?" + Math.random()).then(resp =>
	resp.arrayBuffer()
).then(buf => 
	WebAssembly.instantiate(
		buf,
		imports,
	)
).then(res => {
	mod = res.module;
	inst = res.instance;
	setTimeout(init, 0);
}).catch(err => {
	sysprint("LOADING ERROR: " + err + "\n");
});
