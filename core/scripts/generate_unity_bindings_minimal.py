template_file = """using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
//using Unity.Mathematics;

public class DanceGraphMinimalCpp
{
    public static DanceGraphMinimalCpp CreateAndAwake() { var dll = new DanceGraphMinimalCpp(); dll.Awake(); return dll; }
#if UNITY_EDITOR

    // Handle to the C++ DLL
    public IntPtr libraryHandle;

    public delegate void InitBindingsDelegate(IntPtr debugLog);
    public delegate void RegisterSignalCallbackDelegate(IntPtr callback);
	
	// delegate pairs
	${EDITOR_DELEGATES}

#else
	
#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	static extern void InitBindings(IntPtr debugLog);
    static extern void RegisterSignalCallback(IntPtr cb);

	// player delegates
	${PLAYER_DELEGATES}

#endif

#if UNITY_EDITOR_WIN

    [DllImport("kernel32")]
    public static extern IntPtr LoadLibrary(
        string path);

    [DllImport("kernel32")]
    public static extern IntPtr GetProcAddress(
        IntPtr libraryHandle,
        string symbolName);

    [DllImport("kernel32")]
    public static extern bool FreeLibrary(
        IntPtr libraryHandle);

    public static IntPtr OpenLibrary(string path)
    {
        Debug.Log("Attempting to open native library at " + Application.dataPath);
        IntPtr handle = LoadLibrary(path);
        if (handle == IntPtr.Zero)
        {
            throw new Exception("Couldn't open native library: " + path);
        }
        return handle;
    }

    public static void CloseLibrary(IntPtr libraryHandle)
    {
        FreeLibrary(libraryHandle);
    }

    public static T GetDelegate<T>(
        IntPtr libraryHandle,
        string functionName) where T : class
    {
        IntPtr symbol = GetProcAddress(libraryHandle, functionName);
        if (symbol == IntPtr.Zero)
        {
            throw new Exception("Couldn't get function: " + functionName);
        }
        return Marshal.GetDelegateForFunctionPointer(
            symbol,
            typeof(T)) as T;
    }

#else
 
 
#endif

    /*
        C# functions callable from c++
    */
    delegate void DebugLogDelegate(string str);
    unsafe delegate void ProcessSignalDataDelegate(byte* data, int len, dg.sig.SignalMetadata sigMeta);

#if UNITY_EDITOR_WIN
    const string LIB_PATH = "./Assets/plugins/x86_64/dancegraph-minimal.dll";
#endif

    public void Awake()
    {
#if UNITY_EDITOR

        /*
            c++-call-from-c# declarations as usual
        */
        // Open native library
        libraryHandle = OpenLibrary(LIB_PATH);
        
		InitBindingsDelegate InitBindings = GetDelegate<InitBindingsDelegate>(
            libraryHandle,
            "InitBindings");
            
        RegisterSignalCallbackDelegate RegisterSignalCallback = GetDelegate<RegisterSignalCallbackDelegate>(
            libraryHandle,
            "RegisterSignalCallback");
			
		${BINDINGS}
#else
        

#endif

        // Init C++ library: Call C++ function to register c#-call-from-c++ funcs
        InitBindings(
            Marshal.GetFunctionPointerForDelegate(new DebugLogDelegate(DebugLog))
        );
        
        unsafe
        {
            callback  = new ProcessSignalDataDelegate(ProcessSignalData);    
        }
        fptr = Marshal.GetFunctionPointerForDelegate(callback);
        RegisterSignalCallback( fptr);
    }
    
    private IntPtr fptr;
    private ProcessSignalDataDelegate callback ;

    public void Close()
    {
        Disconnect();
#if UNITY_EDITOR
        CloseLibrary(libraryHandle);
        libraryHandle = IntPtr.Zero;
#endif
    }

    ////////////////////////////////////////////////////////////////
    // C# functions callable from C++
    ////////////////////////////////////////////////////////////////

    static void DebugLog(string str)
    {
        Debug.Log(str);
    }    
    
    unsafe static void ProcessSignalData(byte* data_ptr, int len, dg.sig.SignalMetadata sigMeta)
    {
        //*p *= *p;
        ReadOnlySpan<byte> data = new ReadOnlySpan<byte>((byte*)data_ptr, len);
        dg.sig.CallbackHandler.ProcessSignalData(data, ref sigMeta);
    }
}

public class DanceGraphMinimalCppBehaviour : MonoBehaviour
{
    DanceGraphMinimalCpp dll = new DanceGraphMinimalCpp();
    void Awake() => dll.Awake();
    void OnApplicationQuit() => dll.Close();
}
"""

class bindings_builder : 

    editor_delegates_fmt = """
        public delegate {0} {1}Delegate({2});
        public static {1}Delegate {1};
    """
    player_delegates_fmt = """#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern {0} {1}({2});
    """
    bindings_fmt = """{0} = GetDelegate<{0}Delegate>(
            libraryHandle,
            "{0}");
    """
    def __init__(self):
        self.editor_delegates = []
        self.player_delegates = []
        self.bindings = []
        
    def add_cs2cpp(self, data):
        self.editor_delegates.append( bindings_builder.editor_delegates_fmt.format(data[0],data[1], ", ".join(data[2:])))
        self.player_delegates.append( bindings_builder.player_delegates_fmt.format(data[0],data[1], ", ".join(data[2:])))
        self.bindings.append( bindings_builder.bindings_fmt.format(data[1]))
        
    def add_cpp2cs(self):
        pass
        
    def finalize(self):
        text = template_file.replace("${EDITOR_DELEGATES}", "\n\t".join(self.editor_delegates))\
                            .replace("${PLAYER_DELEGATES}", "\n\t".join(self.player_delegates))\
                            .replace("${BINDINGS}", "\n\t".join(self.bindings))
        return text
        

dll_functions = [
    ('void','Initialize'),
    ('void','Disconnect'),
    ('bool','Connect', 'string name, string serverIp, int serverPort, int localPort, string scene_name, string user_role'),
    ('void','SendSignal', 'byte[] stream, int numBytes, int sigIdx'),
    ('void','Tick'),
]

bb = bindings_builder()
for d in dll_functions:
    bb.add_cs2cpp(d)
text = bb.finalize()
with open("./DanceGraphMinimalCppBehaviour.cs","wt") as fp:
    fp.write( text)