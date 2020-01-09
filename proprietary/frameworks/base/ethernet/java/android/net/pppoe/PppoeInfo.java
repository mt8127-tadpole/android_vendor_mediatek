package android.net.pppoe;

import android.os.Parcel;
import android.os.Parcelable;

/* Just like EthernetDevInfo ?? */
public class PppoeInfo implements Parcelable {
	public String ifname;
	public String ipaddr;
	public String netmask;
	public String route;
	public String dns1;
	public String dns2;

	public PppoeInfo() {
		this.ifname = "";
		this.ipaddr = "";
		this.netmask = "";
		this.route = "";
		this.dns1 = "";
		this.dns2 = "";
	}

    public String toString(){
        return "pppoeInfo: ifname"+ifname
                +",ip="   + ipaddr
                +",mask=" + netmask
                +",route="+ route
                +",dns1=" + dns1
                +",dns2=" + dns2;
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.ifname);
        dest.writeString(this.ipaddr);
        dest.writeString(this.netmask);
        dest.writeString(this.route);
        dest.writeString(this.dns1);
        dest.writeString(this.dns2);
    }

    /** Implement the Parcelable interface {@hide} */
    public static final Creator<PppoeInfo> CREATOR = new Creator<PppoeInfo>() {
        public PppoeInfo createFromParcel(Parcel in) {
            PppoeInfo info = new PppoeInfo();
            info.ifname = in.readString();
            info.ipaddr = in.readString();
            info.netmask = in.readString();
            info.route = in.readString();
            info.dns1 = in.readString();
            info.dns2 = in.readString();
            return info;
        }

        public PppoeInfo[] newArray(int size) {
            return new PppoeInfo[size];
        }
    };
}

