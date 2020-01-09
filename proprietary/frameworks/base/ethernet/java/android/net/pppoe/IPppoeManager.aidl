/**
 * Copyright (c) 2010, The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.net.pppoe;
import android.net.pppoe.IPppoeObserver;
import android.net.pppoe.PppoeInfo;

interface IPppoeManager
{
    void registerObserver(IPppoeObserver obs);
    void unregisterObserver(IPppoeObserver obs);

	void connectPppoe(String account, String password, boolean persist);
    void disconnectPppoe();

    PppoeInfo getPppoeInfo();    
    String getPppoeStatus();
    String getPppoeError();
	
	String getAccount();
    String getPassword();
    void setAccount(String account, String password);
	void setPppoeAutoMode(boolean state);
	boolean getPppoeAutoMode();

    /**
    * @deprecated see  PppoeService.getPppoeInfo()
    * */
    String[] getIpAddress();    
}
