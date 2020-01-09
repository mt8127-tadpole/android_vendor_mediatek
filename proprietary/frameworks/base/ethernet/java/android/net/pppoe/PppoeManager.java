package android.net.pppoe;

import static com.android.internal.util.Preconditions.checkNotNull;

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;
import java.util.ArrayList;

import android.net.pppoe.IPppoeObserver;
import android.net.pppoe.PppoeInfo;
import android.os.SystemProperties;




public class PppoeManager {
	private static final String TAG						 = "PppoeManager";
	private static final String PPPOE_SERVICE            = "pppoe";


	private static final String PPPOE_EVENT_CONNECTED    = "connected";
	private static final String PPPOE_EVENT_DISCONNECT	 = "disconnect";
	private static final String PPPOE_EVENT_CONNECTING	 = "connecting";
	private static final String PPPOE_EVENT_TIMEOUT		 = "timeout";
	private static final String PPPOE_EVENT_AUTH_FAILED  = "auth_failed";
	private static final String PPPOE_EVENT_FAILED		 = "failed";

	public static final String PPPOE_STATE_IDLE          = "idle";
	public static final String PPPOE_STATE_CONNECT       = "connected";
	public static final String PPPOE_STATE_DISCONNECT    = "disconnect";
	public static final String PPPOE_STATE_CONNECTING    = "connecting";
	public static final String PPPOE_STATE_DISCONNECTING = "disconnecting";

	public static final int MSG_PPPOE_IDLE          = 0; 
    public static final int MSG_PPPOE_DIALING       = 1;
    public static final int MSG_PPPOE_CONNECTING    = 2;
    public static final int MSG_PPPOE_CONNECT       = 3;
    public static final int MSG_PPPOE_DISCONNECTING = 4;
    public static final int MSG_PPPOE_DISCONNECT    = 5;

	public static final int MSG_PPPOE_AUTH_FAILED   = 11;
	public static final int MSG_PPPOE_TIME_OUT      = 12;
	public static final int MSG_PPPOE_FAILED        = 0xFF;

    public static final String PPPOE_USERNAME = "pppoe_username";
    public static final String PPPOE_PASSWORD = "pppoe_password";
    public static final String PPPOE_PERSIST  = "pppoe_persist";
    public static final String PPPOE_AUTO     = "pppoe_auto";
   
	private static PppoeManager pppoe;

    private ArrayList<PppoeStatusChangedListener> mStateListeners  = null;
    private IPppoeManager 	  mService;
	private OnRecvMsgListener mListener;
    private static InterfaceObserver mObserver = null;

    public interface OnRecvMsgListener
    {
        public void onRecvMsg(int msg) ;
    }	

    
    public interface PppoeStatusChangedListener{
        public void onNetPppoeStatusChanged(int status, int ErrorCode, String errorInfo);
       
    }

    public static PppoeManager getInstance() {
		if(pppoe == null)
		{
			IBinder b = ServiceManager.getService(PPPOE_SERVICE);
			if (b != null) 
			{
				IPppoeManager service = IPppoeManager.Stub.asInterface(b);
				pppoe = new PppoeManager(service);
			}else{
				Log.w(TAG, "Error getting service name:" + PPPOE_SERVICE);
			}
		}
		 
    	return pppoe;
    }

    private PppoeManager(IPppoeManager service) {
        mService = checkNotNull(service, "missing IPppoeManager");

        mStateListeners = new ArrayList<PppoeStatusChangedListener>();
        try {
            if(mObserver == null){
                mObserver = new InterfaceObserver(this);
                mService.registerObserver(mObserver);
            }
        } catch (RemoteException e) {}
    }

    private  class InterfaceObserver extends IPppoeObserver.Stub {
        private PppoeManager manager;

        InterfaceObserver(PppoeManager pm) {
            super();
            manager = pm;
        }

        public void onStatusChanged(String event, String status) 
		{
			Log.d(TAG, "onStatusChanged, event=" + event + " status=" + status);

			if(manager.mListener != null)
			{ 
			    String errorinfo = SystemProperties.get("net.pppoe.error", "");
				if(event.equals(PPPOE_EVENT_CONNECTED))
				{   
				    updateStateChange(MSG_PPPOE_CONNECT, 0, "ok");
					manager.mListener.onRecvMsg(MSG_PPPOE_CONNECT);
				}
				else if(event.equals(PPPOE_EVENT_DISCONNECT))
				{
				    updateStateChange(MSG_PPPOE_DISCONNECT, 0, errorinfo);
					manager.mListener.onRecvMsg(MSG_PPPOE_DISCONNECT);
				}
				else if(event.equals(PPPOE_EVENT_CONNECTING))
				{
				    updateStateChange(MSG_PPPOE_CONNECTING, 0, "ok");
					manager.mListener.onRecvMsg(MSG_PPPOE_CONNECTING);
				}
				else if(event.equals(PPPOE_EVENT_TIMEOUT))
				{
				    updateStateChange(MSG_PPPOE_FAILED, 11, "timeout");
					manager.mListener.onRecvMsg(MSG_PPPOE_TIME_OUT);
				}
				else if(event.equals(PPPOE_EVENT_AUTH_FAILED))
				{
				    updateStateChange(MSG_PPPOE_FAILED, 12, "auth_failed");
					manager.mListener.onRecvMsg(MSG_PPPOE_AUTH_FAILED);
				}
				else if(event.equals(PPPOE_EVENT_FAILED))
				{
				    updateStateChange(MSG_PPPOE_FAILED, 10, errorinfo);
					manager.mListener.onRecvMsg(MSG_PPPOE_FAILED);
				}
			}
        }
    }

    public void RegisterListener(OnRecvMsgListener listener) {
        
    	this.mListener = listener;
    }

    public void UnregisterListener() {
        
        this.mListener = null;
    }

    public void connectPppoe(String account, String password, boolean persist){
        try {
            mService.connectPppoe(account, password, persist);
        } catch (RemoteException e) {}
    }

    /**
     * @deprecated see PppoeManager.connectPppoe(String account, String password, boolean persist)
     * */
    public void connectPppoe(String account, String password) {
        try {
            mService.connectPppoe(account, password, false);
        } catch (RemoteException e) {}
    }

    public void disconnectPppoe(){
        try {
            mService.disconnectPppoe();
		} catch (RemoteException e) {}
    }

    public String getPppoeStatus(){
        try {
            return mService.getPppoeStatus();
		} catch (RemoteException e) {
			return null;
		}
    }

	public PppoeInfo getPppoeInfo(){
        try {
            return mService.getPppoeInfo();
		} catch (RemoteException e) {
			return null;
		}
	}

	public String getPppoeError(){
        try {
            return mService.getPppoeError();
		} catch (RemoteException e) {
			return null;
		}
	}	

    /**
     * @deprecated see  PppoeManager.getPppoeInfo()
     * */
	public String[] getIpAddress(){
        try {
            return mService.getIpAddress();
		} catch (RemoteException e) {
			return null;
		}
	}


    ///M add by mtk94127, add pppoe  API @{
    /**
         * @config the pppoe dial is auto or not 
         */
    public boolean setPPPoEAutoDial(boolean state) {
        try {
            mService.setPppoeAutoMode(state);
            return true;
		} catch (RemoteException e) {
			return false;
		}

    }
    
    /**
         * @get the infomation of  pppoe dial is auto or not 
         */
    public boolean getPPPoEAutoDial() {
        try {
            return mService.getPppoeAutoMode();
		} catch (RemoteException e) {
			return false;
		}

    }
    
    /**
         * @get the account 
         */
    public String getPPPoEAccount() {
        try {
            return mService.getAccount();
		} catch (RemoteException e) {
			return null;
		}

    }

    /**
         * @get the password 
         */
    public String getPPPoEPassword() {
        try {
            return mService.getPassword();
		} catch (RemoteException e) {
			return null;
		}

    }

    /**
         * @pppoe dial up
         */
    public boolean PPPoEDial(String account, String password) {
        try {
            mService.connectPppoe(account, password, false);
            return true;
        } catch (RemoteException e) {
            return false;
        }

    }

    /**
         * @set  the account and  password 
         */
    public boolean setPPPoEAccount(String account, String password) {
        try {
            mService.setAccount(account, password);
            return true;
        } catch (RemoteException e) {
            return false;
        }

    }
    
    /**
         * @disconnect the pppoe connection
         */
    public boolean PPPoEDisconnect() {
        try {
            mService.disconnectPppoe();
            return true;
		} catch (RemoteException e) {
		    return false;
		}
    }
    
    /**
         * @get pppoe state
         */
    public int getPPPoEState() {
        String pppoestate = getPppoeStatus();

        if(pppoestate.equals(PPPOE_STATE_IDLE)) {

            return MSG_PPPOE_IDLE;  
        } else if(pppoestate.equals(PPPOE_STATE_CONNECTING)) {
        
            return  MSG_PPPOE_CONNECTING;
        } else if(pppoestate.equals(PPPOE_STATE_CONNECT)) {
        
            return MSG_PPPOE_CONNECT;
        } else if(pppoestate.equals(PPPOE_STATE_DISCONNECTING)) {

            return MSG_PPPOE_DISCONNECT;
        } else if(pppoestate.equals(PPPOE_STATE_DISCONNECT)) {

            return MSG_PPPOE_DISCONNECT;
        } else {
        
            return MSG_PPPOE_FAILED;
        }
            
        
    }

    private  void updateStateChange(int status, int ErrorCode, String errorInfo) {
        if (mStateListeners == null) {
            return ;
        }

        Log.d(TAG, "updateStateChange, status =" + status + " ErrorCode =" + ErrorCode + "errorInfo =" +errorInfo);
        for (PppoeStatusChangedListener mListener : mStateListeners) {
            mListener.onNetPppoeStatusChanged(status, ErrorCode, errorInfo);
        }
    }

    /**
         * @add status listener client
         */
    public void setStatusListener(PppoeStatusChangedListener listener) {
    
        mStateListeners.add(listener);
    }
    
    /**
         * @remove status listener client
         */
    public void unsetStatusListener(PppoeStatusChangedListener listener) {
        if (mStateListeners != null) {

            mStateListeners.remove(listener);
        }
    }
    ///M @}
}

