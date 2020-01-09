package com.android.server;

import static android.Manifest.permission.ACCESS_NETWORK_STATE;
import static android.Manifest.permission.CHANGE_NETWORK_STATE;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
//import static android.provider.Settings.Secure.NETSTATS_ENABLED;
import static com.android.server.NetworkManagementSocketTagger.PROP_QTAGUID_ENABLED;

import com.android.server.NativeDaemonConnector;
import com.android.server.NativeDaemonConnectorException;


import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;

import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.RemoteException;
import android.os.RemoteCallbackList;
import android.os.ServiceManager;
import android.os.INetworkManagementService;

import android.net.NetworkInfo;
import android.net.RouteInfo;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.LinkCapabilities;
import android.net.NetworkUtils;
import android.net.InterfaceConfiguration;

import android.net.IConnectivityManager;
import android.net.ConnectivityManager;
import android.net.INetworkManagementEventObserver;
import android.net.wifi.IWifiManager;
import android.net.pppoe.PppoeManager;
import android.net.pppoe.IPppoeManager;
import android.net.pppoe.IPppoeObserver;
import android.net.pppoe.PppoeInfo;
import android.net.ethernet.IEthernetManager;
import android.net.ethernet.EthernetManager;
import android.net.ethernet.EthernetDevInfo;

import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import java.net.InetAddress;
import java.net.Inet4Address;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;

// added by mtk94127
import android.content.IntentFilter;
import android.os.SystemProperties;




public class PppoeService extends IPppoeManager.Stub {
	
	private static final String TAG 		= "PppoeService";
	private static final String PPPOE_TAG   = "PppoeConnector";
	private static final String DEFAULT_DEV = "eth0";

	private static final String PPPOE_EVENT_CONNECTED    = "connected";
	private static final String PPPOE_EVENT_DISCONNECT	 = "disconnect";
	private static final String PPPOE_EVENT_CONNECTING	 = "connecting";
	private static final String PPPOE_EVENT_TIMEOUT		 = "timeout";
	private static final String PPPOE_EVENT_AUTH_FAILED  = "auth_failed";
	private static final String PPPOE_EVENT_FAILED		 = "failed";

	private static final String PPPOE_STATE_IDLE          = "idle";
	private static final String PPPOE_STATE_DISCONNECT    = "disconnect";
	private static final String PPPOE_STATE_CONNECTED     = "connected";
	private static final String PPPOE_STATE_CONNECTING    = "connecting";
    private static final String PPPOE_STATE_DISCONNECTING = "disconnecting";

    class PppoeResponseCode {
        public static final int CommandOkay     = 200;
        public static final int OperationFailed = 400;
        public static final int StautsChanged   = 680;
    }

    private Context mContext;

	private	INetworkManagementService netdService;
    private IEthernetManager 	 ethernetManager;
    private IWifiManager		 wifiManager ;
    private EthernetDevInfo 	 savedInfo;
	
	private InterfaceObserver 	  mInterfaceObserver;

	private NativeDaemonConnector mConnector;
	private Thread mThread;
	
	private String  mStatus    = PPPOE_STATE_IDLE;
	private boolean mTriggered = false;
	private String  mErrMsg    = "OK";

    private static String mIface = "";
    private static boolean mLinkUp = false;

	private PppoeInfo mInfo    = new PppoeInfo();
	private Collection<RouteInfo> mRoutes = new ArrayList<RouteInfo>();

    private NetworkInfo mNetworkInfo;
    private LinkProperties mLinkProperties;
    private LinkCapabilities mLinkCapabilities;

    private boolean mPersist;
    private String mAccount = "";
    private String mPassword = "";
    private boolean mAuotMode;

	final RemoteCallbackList<IPppoeObserver> mWatchers = new RemoteCallbackList<IPppoeObserver>();
	
    public PppoeService() {
        IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        netdService = INetworkManagementService.Stub.asInterface(b);
		
		IBinder eth = ServiceManager.getService("ethernetservice");
        ethernetManager = IEthernetManager.Stub.asInterface(eth);
		
        IBinder wifi = ServiceManager.getService(Context.WIFI_SERVICE);
        wifiManager = IWifiManager.Stub.asInterface(eth);

        mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_ETHERNET, 0, "ETH", "PPPOE");
        mLinkProperties = new LinkProperties();
        mLinkCapabilities = new LinkCapabilities();

        mPersist = false;
        



	}


	public void startMonitoring(Context context, Handler handler){
		Log.d(TAG, "startMonitoring ");


        mContext = context;
        mPersist = getPersistState();

        final ContentResolver cr = mContext.getContentResolver();
        mAccount  = Settings.Secure.getString(cr, PppoeManager.PPPOE_USERNAME);
        mPassword = Settings.Secure.getString(cr, PppoeManager.PPPOE_PASSWORD);

        String autostate = Settings.Secure.getString(cr, PppoeManager.PPPOE_AUTO);
        if (autostate == null || autostate.equals("") || autostate.equals("false")) {
            mAuotMode = false;
        } else if (autostate.equals("true")) {
            mAuotMode = true;
        }
 
        startMonitoringAction(); 		
  
		/* Monitor pppoe event */
		mConnector = new NativeDaemonConnector(new PppoeCallbackReceiver(), "pppoesock", 10, PPPOE_TAG, 160);
		mThread = new Thread(mConnector, PPPOE_TAG);
		mThread.start();

        if (mAuotMode && isPPPoEMode()) {
            Log.d(TAG, " auto connect pppoe");
            AutoConnect();
        }
	}

	
	public void startMonitoringAction() {
		Log.d(TAG, "startMonitoringAction ");


		/* Monitor cable plugin and plugout */
		mInterfaceObserver = new InterfaceObserver(this);
		String sIfaceMatch = mContext.getResources().getString(com.android.internal.R.string.config_ethernet_iface_regex);
        Log.d(TAG, "sIfaceMatch: " + sIfaceMatch);
        try {
            final String[] ifaces = netdService.listInterfaces();
            for (String iface : ifaces) {
                Log.d(TAG, "iface: " + iface);
                if (iface.matches(sIfaceMatch)) {
                    mIface = iface;
                    Log.d(TAG, " mIface: " + mIface);
                    InterfaceConfiguration config = netdService.getInterfaceConfig(iface);
                    mLinkUp = config.isActive();

                    if (mLinkUp) {
                        Log.d(TAG, "mLinkUp is true ");
                    }
                    break;
                }
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Could not get list of interfaces " + e);
        }
		
		try {
			netdService.registerObserver(mInterfaceObserver);
		} catch (RemoteException e) {
			Log.e(TAG, "Could not register InterfaceObserver " + e);
		}
	}

    public void registerObserver(IPppoeObserver obs)
	{
        synchronized (this) {
            mWatchers.register(obs);
        }
	}

    public void unregisterObserver(IPppoeObserver obs)
	{
        synchronized (this) {
            mWatchers.unregister(obs);
        }
	}

    public boolean getPersistState(){
        final ContentResolver cr = mContext.getContentResolver();
        String isPersist = Settings.Secure.getString(cr, PppoeManager.PPPOE_PERSIST);
		if(isPersist != null){
			return isPersist.equals("on");
		}else{
			Settings.Secure.putString(cr, PppoeManager.PPPOE_PERSIST, "off");
			return false;
		}
    }

    private boolean isPPPoEMode(){
        final ContentResolver cr = mContext.getContentResolver();
        String mode = Settings.Secure.getString(cr, EthernetManager.ETHERNET_MODE);
        if(mode != null){
            return mode.equals(EthernetDevInfo.ETHERNET_CONN_MODE_PPPOE);
        }

        return false;
    }

    private void AutoConnect(){
        connectPppoe(mAccount, mPassword, true);
    }

    public void connectPppoe(String account, String password, boolean persist){
        synchronized (this) {
            if(account != null && password != null)
            {
                if(mLinkUp)
                {
                    if(!mTriggered)
                    {
                        Log.d(TAG, "connectPppoe: dialup start");

                        mErrMsg   = "OK";
                        mPersist  = persist;
                        mAccount  = account;
                        mPassword = password;
                        mTriggered = true;

                        backupAndRemoveDefaultRoute();

                        try {
                            mConnector.execute("pppoe", "dialup", DEFAULT_DEV, account, password);
                        } catch (NativeDaemonConnectorException e) {
                            throw e.rethrowAsParcelableException();
                        }

                        Log.d(TAG, "connectPppoe: dialup end");
                    }
                }else{
                    mStatus = PPPOE_STATE_DISCONNECT;
                    mErrMsg = "link down";
                    broadcastConnectionStatus(PPPOE_EVENT_FAILED);
                }
            }
        }
    }

    public void disconnectPppoe(){
        synchronized (this) {
			if(mTriggered)
			{
				Log.d(TAG, "disconnectPppoe: hangup start, mLinkUp = " + mLinkUp);

				mErrMsg = "OK";
                
                try {
                    mConnector.execute("pppoe", "hangup", mIface);
                } catch (NativeDaemonConnectorException e) {
                    throw e.rethrowAsParcelableException();
                }

				mStatus  = PPPOE_STATE_DISCONNECT;
				mTriggered = false;

				recoverDefaultRoute();

				if(mLinkUp)
				{
					updateConnectionStatus(PPPOE_EVENT_DISCONNECT, "hang up");
	            }else{
					updateConnectionStatus(PPPOE_EVENT_DISCONNECT, "link down");
		        }
                broadcastConnectionStatus(PPPOE_EVENT_DISCONNECT);

				Log.d(TAG, "disconnectPppoe: hangup end");
			}
			else
			{
				mErrMsg = "OK";
				mStatus = PPPOE_STATE_DISCONNECT;			
				broadcastConnectionStatus(PPPOE_EVENT_DISCONNECT);
			}
    	}
    }

	public PppoeInfo getPppoeInfo(){
		return mInfo;
	}

    public String getPppoeStatus(){
		return mStatus;
    }

	public String getPppoeError(){
		return mErrMsg;
	}
	
    /**
    	* @deprecated see  PppoeService.getPppoeInfo()     
    	* */
    public String[] getIpAddress(){	
		String[] ret = new String[5];
		ret[0] = mInfo.ipaddr;
		ret[1] = mInfo.netmask;
		ret[2] = mInfo.route;
		ret[3] = mInfo.dns1;
		ret[4] = mInfo.dns2;
		
		return ret;
    }

	public ArrayList<String> getDevices(){
		ArrayList<String> list = new ArrayList<String>();
		try {
			String[] devs = netdService.listInterfaces();
			for(String dev : devs)
			{			 
				list.add(dev);
			}
        } catch (RemoteException e) {
            Log.e(TAG, "Could not get list of interfaces " + e);
        }
		
		return list;
	}

    public String getAccount() {
        
        return mAccount;
    }

    public String getPassword() {
        
        return mPassword;
    }

    public void setAccount(String account, String password) {

        mAccount = account;
        mPassword = password;
        
        final ContentResolver cr = mContext.getContentResolver();
        Settings.Secure.putString(cr, PppoeManager.PPPOE_USERNAME, mAccount);
        Settings.Secure.putString(cr, PppoeManager.PPPOE_PASSWORD, mPassword);

    }

    public void setPppoeAutoMode(boolean state) {

        Log.d(TAG, "setPppoeAutoMode:" + state);
        if (mAuotMode != state) {
            mAuotMode = state;
            final ContentResolver cr = mContext.getContentResolver();
            if (state) {    
                Settings.Secure.putString(cr, PppoeManager.PPPOE_AUTO, "true");
            } else {
                Settings.Secure.putString(cr, PppoeManager.PPPOE_AUTO, "false");
            }
        }

        if (mAuotMode && isPPPoEMode()) {
            Log.d(TAG, "setPppoeAutoMode and auto connect pppoe");
            AutoConnect();
        }
    }

    public boolean getPppoeAutoMode() {
        
        return mAuotMode;
    }    
        
	private void backupAndRemoveDefaultRoute(){
		Log.d(TAG, "backupAndRemoveDefaultRoute");
		
		mRoutes.clear();

		try {
			RouteInfo[] routes = netdService.getRoutes(mIface);
			for(RouteInfo r : routes)
			{
				if(r.isDefaultRoute())
				{
					mRoutes.add(r);
					netdService.removeRoute(mIface, r);
				}
			}
        } catch (RemoteException e) {
		    e.printStackTrace();
        }
	}

	private void recoverDefaultRoute(){
		Log.d(TAG, "recoverDefaultRoute");

		try {
			for(RouteInfo r : mRoutes){
				netdService.addRoute(mIface, r);
			}
        } catch (RemoteException e) {
	        e.printStackTrace();
        }

		mRoutes.clear();
	}
	
    private void backupPreviousNetworkInfo() {
        try {
			IBinder connect = ServiceManager.getService(Context.CONNECTIVITY_SERVICE);
			IConnectivityManager connectivityService = IConnectivityManager.Stub.asInterface(connect);
			
            NetworkInfo info = connectivityService.getActiveNetworkInfo();
            if(info != null && info.getTypeName().equals("ETH")) {
                savedInfo = new EthernetDevInfo();
                try {
                    savedInfo = ethernetManager.getSavedConfig();
                } catch (RemoteException e) {
                    savedInfo.setIfName(mIface);
                    savedInfo.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP);
                    e.printStackTrace();
                }
            }else{
                savedInfo = null;
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private void recoverPreviousNetworkInfo(){
        try {
            if (savedInfo != null)
            {
                ethernetManager.updateDevInfo(savedInfo);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private void broadcastConnectionInfo() {
        Log.d(TAG, "  broadcastConnectionInfo");

        final Intent intent = new Intent(EthernetManager.NETWORK_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);

        intent.putExtra(EthernetManager.EXTRA_NETWORK_INFO, mNetworkInfo);
        intent.putExtra(EthernetManager.EXTRA_LINK_PROPERTIES, mLinkProperties);
        intent.putExtra(EthernetManager.EXTRA_LINK_CAPABILITIES, mLinkCapabilities);

        mContext.sendStickyBroadcast(intent);
    }

    private void updateNetworkInfo(boolean state){
        if(state){
            mNetworkInfo.setDetailedState(NetworkInfo.DetailedState.CONNECTED, null, null);
        }else{
            mNetworkInfo.setDetailedState(NetworkInfo.DetailedState.DISCONNECTED, null, null);
        }

        mNetworkInfo.setIsAvailable(state);
    }

    private void updateLinkProperties(boolean state) {
        mLinkProperties.clear();
        if(state){
            mLinkProperties.setInterfaceName(mInfo.ifname);
            InetAddress destAddr = NetworkUtils.numericToInetAddress(mInfo.ipaddr);
            InetAddress maskAddr = NetworkUtils.numericToInetAddress(mInfo.netmask);
            int prefixLength = NetworkUtils.netmaskIntToPrefixLength(NetworkUtils.inetAddressToInt((Inet4Address)maskAddr));
            LinkAddress linkAddress = new LinkAddress(destAddr, prefixLength);
            mLinkProperties.addLinkAddress(linkAddress);

            if (!TextUtils.isEmpty(mInfo.dns1)) {
                InetAddress dns1Addr = NetworkUtils.numericToInetAddress(mInfo.dns1);
                mLinkProperties.addDns(dns1Addr);
            }

            if (!TextUtils.isEmpty(mInfo.dns2)) {
                InetAddress dns2Addr = NetworkUtils.numericToInetAddress(mInfo.dns2);
                mLinkProperties.addDns(dns2Addr);
            }

            InetAddress gatewayAddr = NetworkUtils.numericToInetAddress(mInfo.route);
            RouteInfo route = new RouteInfo(null, gatewayAddr);
            mLinkProperties.addRoute(route);
        }
    }

    private void broadcastConnectionStatus(String event){
        int i = mWatchers.beginBroadcast();
        while (i > 0) {
            i--;
            IPppoeObserver w = mWatchers.getBroadcastItem(i);
            if (w != null) {
                try {
                    w.onStatusChanged(event, mStatus);
                } catch (RemoteException e) {
	                e.printStackTrace();
                }
            }
        }
        mWatchers.finishBroadcast();
	}

	private void updateConnectionStatus(String event, String args){
        Log.d(TAG, "  updateConnectionStatus");

        if(event.equals(PPPOE_EVENT_CONNECTING))
        {
            mStatus = PPPOE_STATE_CONNECTING;
        }
        else if(event.equals(PPPOE_EVENT_CONNECTED))
		{
            final ContentResolver cr = mContext.getContentResolver();
            if(mPersist){
                Settings.Secure.putString(cr, PppoeManager.PPPOE_PERSIST, "on");
                Settings.Secure.putString(cr, PppoeManager.PPPOE_USERNAME, mAccount);
                Settings.Secure.putString(cr, PppoeManager.PPPOE_PASSWORD, mPassword);
            }else{
                Settings.Secure.putString(cr, PppoeManager.PPPOE_PERSIST, "off");
                Settings.Secure.putString(cr, PppoeManager.PPPOE_USERNAME, "");
                Settings.Secure.putString(cr, PppoeManager.PPPOE_PASSWORD, "");
            }

            String[] s = args.split("#");
            mInfo.ifname  = s[0];
			mInfo.ipaddr  = s[1];
			mInfo.netmask = s[2];
			mInfo.route   = s[3];
			mInfo.dns1    = s[4];
			mInfo.dns2    = s[5];

			mStatus = PPPOE_STATE_CONNECTED;
			mErrMsg = "OK";

			backupPreviousNetworkInfo() ;

            updateNetworkInfo(true);
            updateLinkProperties(true);
            broadcastConnectionInfo();
		}
		else if(event.equals(PPPOE_EVENT_DISCONNECT) || event.equals(PPPOE_EVENT_TIMEOUT)
				|| event.equals(PPPOE_EVENT_AUTH_FAILED) || event.equals(PPPOE_EVENT_FAILED))
		{
            mInfo.ifname  = "";
			mInfo.ipaddr  = "";
			mInfo.netmask = "";
			mInfo.route  = "";
			mInfo.dns1    = "";
			mInfo.dns2    = "";

			mStatus = PPPOE_STATE_DISCONNECT;
			mErrMsg = args;

            mTriggered = false;

            updateNetworkInfo(false);
            updateLinkProperties(false);
            broadcastConnectionInfo();

			recoverPreviousNetworkInfo();
		}
	}

    private static class InterfaceObserver extends INetworkManagementEventObserver.Stub {
        private PppoeService mService;
		
        InterfaceObserver(PppoeService service) {
            super();
            mService = service;
        }

        public void interfaceStatusChanged(String iface, boolean up) {
            Log.d(TAG, "Interface status changed: " + iface + (up ? "up" : "down"));
        }

        public void interfaceLinkStateChanged(String iface, boolean up) {
            if (mIface.equals(iface) && mLinkUp != up) {
                Log.d(TAG, "Interface " + iface + " link " + (up ? "up" : "down"));
                mLinkUp = up;

                if(mLinkUp){
                    if(mService.isPPPoEMode() && mService.getPersistState()){
                        mService.AutoConnect();
                    }
                }else{
                    if(mService.mTriggered){
                        mService.disconnectPppoe();
                    }
                }
            }
        }

        public void interfaceAdded(String iface) {
            //mService.interfaceAdded(iface);
        }

        public void interfaceRemoved(String iface) {
            //mService.interfaceRemoved(iface);
        }

        public void limitReached(String limitName, String iface) {
            // Ignored.
        }

        public void interfaceClassDataActivityChanged(String label, boolean active) {
            // Ignored.
        }
        
        public void addressUpdated(String address, String iface, int flags, int scope) {}

        public void addressRemoved(String address, String iface, int flags, int scope) {}

    }

    class PppoeCallbackReceiver implements INativeDaemonConnectorCallbacks {

        /** {@inheritDoc} */
        public void onDaemonConnected() {
        
        }

        /** {@inheritDoc} */
        public boolean onEvent(int code, String raw, String[] cooked) {
	        Log.d(TAG, "PppoeCallbackReceiver raw=" + raw);
            switch (code) {
            case PppoeResponseCode.StautsChanged :
                    Log.d(TAG, "pppoe connect event:" + cooked[2]);
                    if (cooked.length < 4 || !cooked[1].equals("pppoe")) {
                        throw new IllegalStateException(String.format("Invalid event from daemon (%s)", raw));
                    }
					
                    if (cooked[2].equals(PPPOE_EVENT_CONNECTED)
						|| cooked[2].equals(PPPOE_EVENT_DISCONNECT)
						|| cooked[2].equals(PPPOE_EVENT_CONNECTING)
						|| cooked[2].equals(PPPOE_EVENT_TIMEOUT)
						|| cooked[2].equals(PPPOE_EVENT_AUTH_FAILED)
						|| cooked[2].equals(PPPOE_EVENT_FAILED)) 
					{	
                        updateConnectionStatus(cooked[2], cooked[3]);
						broadcastConnectionStatus(cooked[2]);
                        return true;
                	}
                    throw new IllegalStateException(
                            String.format("Invalid event from daemon (%s)", raw));
            default: 
				break;
            }
			
            return false;
        }
    }
}
