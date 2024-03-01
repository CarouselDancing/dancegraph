using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
//using Unity.Mathematics;

public class DanceGraphCpp
{
    public static DanceGraphCpp CreateAndAwake() { var dll = new DanceGraphCpp(); dll.Awake(); return dll; }
#if UNITY_EDITOR

    // Handle to the C++ DLL
    public IntPtr libraryHandle;

    public delegate void InitBindingsDelegate(IntPtr debugLog);
	
	// delegate pairs
	
        public delegate void InitializeDelegate();
        public static InitializeDelegate Initialize;
    
	
        public delegate void ConnectDelegate(string name, string serverIp, int serverPort, int localPort);
        public static ConnectDelegate Connect;
    
	
        public delegate void SignalCallback_HmdDelegate([In] float[] xform);
        public static SignalCallback_HmdDelegate SignalCallback_Hmd;
    
	
        public delegate void SignalCallback_ZedDelegate([In] float[] zeddata);
        public static SignalCallback_ZedDelegate SignalCallback_Zed;
    
	
        public delegate bool GetLocalZedDataDelegate([Out] UnityZedData zeddata);
        public static GetLocalZedDataDelegate GetLocalZedData;
    
	
        public delegate void TickDelegate();
        public static TickDelegate Tick;
    
	
        public delegate bool ReadLocalZedDataDelegate();
        public static ReadLocalZedDataDelegate ReadLocalZedData;
    
	
        public delegate int GetLocalZedBodyCountDelegate();
        public static GetLocalZedBodyCountDelegate GetLocalZedBodyCount;
    
	
        public delegate double GetLocalZedElapsedDelegate();
        public static GetLocalZedElapsedDelegate GetLocalZedElapsed;
    
	
        public delegate void GetLocalZedRootTransformDelegate(float[] rootT);
        public static GetLocalZedRootTransformDelegate GetLocalZedRootTransform;
    
	
        public delegate void GetLocalZedBoneDataDelegate(float[] boneD);
        public static GetLocalZedBoneDataDelegate GetLocalZedBoneData;
    
	
        public delegate void GetLocalZedBodyIDsDelegate(int[] idData);
        public static GetLocalZedBodyIDsDelegate GetLocalZedBodyIDs;
    
	
        public delegate int GetUserIDDelegate();
        public static GetUserIDDelegate GetUserID;
    
	
        public delegate int GetPacketIDDelegate();
        public static GetPacketIDDelegate GetPacketID;
    
	
        public delegate ulong GetPacketTimestampDelegate();
        public static GetPacketTimestampDelegate GetPacketTimestamp;
    
	
        public delegate int GetDataSizeDelegate();
        public static GetDataSizeDelegate GetDataSize;
    

#else
	
#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	static extern void InitBindings(IntPtr debugLog);

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
	public static extern void Connect(string name, string serverIp, int serverPort, int localPort);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void SignalCallback_Hmd([In] float[] xform);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void SignalCallback_Zed([In] float[] zeddata);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern bool GetLocalZedData([Out] UnityZedData zeddata);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void Tick();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern bool ReadLocalZedData();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern int GetLocalZedBodyCount();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern double GetLocalZedElapsed();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void GetLocalZedRootTransform(float[] rootT);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void GetLocalZedBoneData(float[] boneD);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern void GetLocalZedBodyIDs(int[] idData);
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern int GetUserID();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern int GetPacketID();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern ulong GetPacketTimestamp();
    
	#if UNITY_WEBGL
	[DllImport ("__Internal")]
#else
	[DllImport ("dancegraph")]
#endif
	public static extern int GetDataSize();
    

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

#if UNITY_EDITOR_WIN
    const string LIB_PATH = "./Assets/plugins/x86_64/dancegraph.dll";
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
			
		Initialize = GetDelegate<InitializeDelegate>(
            libraryHandle,
            "Initialize");
    
	Connect = GetDelegate<ConnectDelegate>(
            libraryHandle,
            "Connect");
    
	SignalCallback_Hmd = GetDelegate<SignalCallback_HmdDelegate>(
            libraryHandle,
            "SignalCallback_Hmd");
    
	SignalCallback_Zed = GetDelegate<SignalCallback_ZedDelegate>(
            libraryHandle,
            "SignalCallback_Zed");
    
	GetLocalZedData = GetDelegate<GetLocalZedDataDelegate>(
            libraryHandle,
            "GetLocalZedData");
    
	Tick = GetDelegate<TickDelegate>(
            libraryHandle,
            "Tick");
    
	ReadLocalZedData = GetDelegate<ReadLocalZedDataDelegate>(
            libraryHandle,
            "ReadLocalZedData");
    
	GetLocalZedBodyCount = GetDelegate<GetLocalZedBodyCountDelegate>(
            libraryHandle,
            "GetLocalZedBodyCount");
    
	GetLocalZedElapsed = GetDelegate<GetLocalZedElapsedDelegate>(
            libraryHandle,
            "GetLocalZedElapsed");
    
	GetLocalZedRootTransform = GetDelegate<GetLocalZedRootTransformDelegate>(
            libraryHandle,
            "GetLocalZedRootTransform");
    
	GetLocalZedBoneData = GetDelegate<GetLocalZedBoneDataDelegate>(
            libraryHandle,
            "GetLocalZedBoneData");
    
	GetLocalZedBodyIDs = GetDelegate<GetLocalZedBodyIDsDelegate>(
            libraryHandle,
            "GetLocalZedBodyIDs");
    
	GetUserID = GetDelegate<GetUserIDDelegate>(
            libraryHandle,
            "GetUserID");
    
	GetPacketID = GetDelegate<GetPacketIDDelegate>(
            libraryHandle,
            "GetPacketID");
    
	GetPacketTimestamp = GetDelegate<GetPacketTimestampDelegate>(
            libraryHandle,
            "GetPacketTimestamp");
    
	GetDataSize = GetDelegate<GetDataSizeDelegate>(
            libraryHandle,
            "GetDataSize");
    
#else
        

#endif

        // Init C++ library: Call C++ function to register c#-call-from-c++ funcs
        InitBindings(
            Marshal.GetFunctionPointerForDelegate(new DebugLogDelegate(DebugLog))
        );
    }

    public void Close()
    {
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
}

public class DanceGraphCppBehaviour : MonoBehaviour
{
    DanceGraphCpp dll = new DanceGraphCpp();
    void Awake() => dll.Awake();
    void OnApplicationQuit() => dll.Close();
}
