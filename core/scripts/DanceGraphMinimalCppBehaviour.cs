using System.Collections;
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
	
        public delegate void InitializeDelegate();
        public static InitializeDelegate Initialize;
    
	
        public delegate void DisconnectDelegate();
        public static DisconnectDelegate Disconnect;
    
	
        public delegate bool ConnectDelegate(string name, string serverIp, int serverPort, int localPort, string scene_name, string user_role);
        public static ConnectDelegate Connect;
    
	
        public delegate void SendSignalDelegate(byte[] stream, int numBytes, int sigIdx);
        public static SendSignalDelegate SendSignal;
    
	
        public delegate void TickDelegate();
        public static TickDelegate Tick;
    

#else
	
#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	static extern void InitBindings(IntPtr debugLog);
    static extern void RegisterSignalCallback(IntPtr cb);

	// player delegates
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void Initialize();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void Disconnect();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern bool Connect(string name, string serverIp, int serverPort, int localPort, string scene_name, string user_role);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void SendSignal(byte[] stream, int numBytes, int sigIdx);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void Tick();
    

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
			
		Initialize = GetDelegate<InitializeDelegate>(
            libraryHandle,
            "Initialize");
    
	Disconnect = GetDelegate<DisconnectDelegate>(
            libraryHandle,
            "Disconnect");
    
	Connect = GetDelegate<ConnectDelegate>(
            libraryHandle,
            "Connect");
    
	SendSignal = GetDelegate<SendSignalDelegate>(
            libraryHandle,
            "SendSignal");
    
	Tick = GetDelegate<TickDelegate>(
            libraryHandle,
            "Tick");
    
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
