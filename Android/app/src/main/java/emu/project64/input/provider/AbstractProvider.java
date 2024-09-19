package emu.project64.input.provider;

import java.util.ArrayList;

import emu.project64.AndroidDevice;
import android.annotation.TargetApi;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * The base class for transforming arbitrary input data into a common format.
 * 
 * @see KeyProvider
 * @see AxisProvider
 * @see SensorProvider
 * @see InputMap
 */
public abstract class AbstractProvider
{
    /** The offset used to construct the hardware ID for a MOGA controller. */
    private static final int HARDWARE_ID_MOGA_OFFSET = 1000;
    
    /** The maximum possible hardware ID for a MOGA controller, inclusive. */
    private static final int HARDWARE_ID_MOGA_MAX = 1010;
    
    /**
     * The interface for listening to a provider.
     */
    public interface OnInputListener
    {
        /**
         * Called when a single input has been dispatched.
         * 
         * @param inputCode  The universal input code that was dispatched.
         * @param strength   The input strength, between 0 and 1, inclusive.
         * @param hardwareId The identifier of the source device.
         */
        public void onInput( int inputCode, float strength, int hardwareId );
        
        /**
         * Called when multiple inputs have been dispatched simultaneously.
         * 
         * @param inputCodes The universal input codes that were dispatched.
         * @param strengths  The input strengths, between 0 and 1, inclusive.
         * @param hardwareId The identifier of the source device.
         */
        public void onInput( int[] inputCodes, float[] strengths, int hardwareId );
    }
    
    /** The strength threshold above which an input is said to be "on". */
    public static final float STRENGTH_THRESHOLD = 0.5f;
    
    /** Listener management. */
    private final ArrayList<AbstractProvider.OnInputListener> mPublisher;
    
    /**
     * Instantiates a new abstract provider.
     */
    protected AbstractProvider()
    {
        mPublisher = new ArrayList<AbstractProvider.OnInputListener>();
    }
    
    /**
     * Registers a listener to start receiving input notifications.
     * 
     * @param listener The listener to register. Null values are safe.
     */
    public void registerListener( AbstractProvider.OnInputListener listener )
    {
        if( ( listener != null ) && !mPublisher.contains( listener ) )
        {
        	mPublisher.add( listener );
        }
    }
    
    /**
     * Unregisters a listener to stop receiving input notifications.
     * 
     * @param listener The listener to unregister. Null values are safe.
     */
    public void unregisterListener( AbstractProvider.OnInputListener listener )
    {
        if( listener != null )
        {
        	mPublisher.remove( listener );
        }
    }
    
    /**
     * Unregisters all listeners.
     */
    public void unregisterAllListeners()
    {
    	mPublisher.clear();            
    }
    
    /**
     * Obtains unique hardware id from an input event.
     * 
     * @param event The event generated by the hardware.
     * 
     * @return The unique identifier of the hardware.
     */
    public static int getHardwareId( KeyEvent event )
    {
        // This might be replaced by something else in the future... so we abstract it
        return event.getDeviceId();
    }
    
    /**
     * Obtains unique hardware id from an input event.
     * 
     * @param event The event generated by the hardware.
     * 
     * @return The unique identifier of the hardware.
     */
    public static int getHardwareId( MotionEvent event )
    {
        // This might be replaced by something else in the future... so we abstract it
        return event.getDeviceId();
    }
    
    /**
     * Obtains unique hardware id from an input event.
     * 
     * @param event The event generated by the hardware.
     * 
     * @return The unique identifier of the hardware.
     */
    public static int getHardwareId( com.bda.controller.KeyEvent event )
    {
        // This might be replaced by something else in the future... so we abstract it
        return event.getControllerId() + HARDWARE_ID_MOGA_OFFSET;
    }
    
    /**
     * Obtains unique hardware id from an input event.
     * 
     * @param event The event generated by the hardware.
     * 
     * @return The unique identifier of the hardware.
     */
    public static int getHardwareId( com.bda.controller.MotionEvent event )
    {
        // This might be replaced by something else in the future... so we abstract it
        return event.getControllerId() + HARDWARE_ID_MOGA_OFFSET;
    }
    
    /**
     * Determines whether the hardware is available. This is a conservative test in that it will
     * return true if the availability cannot be conclusively determined.
     * 
     * @param id The unique hardware identifier.
     * 
     * @return True if the associated hardware is available or indeterminate.
     */
    @TargetApi( 9 )
    public static boolean isHardwareAvailable( int id )
    {
        // This might be replaced by something else in the future... so we abstract it
        if( id > HARDWARE_ID_MOGA_OFFSET && id <= HARDWARE_ID_MOGA_MAX )
        {
            return true;
        }
        else if( AndroidDevice.IS_GINGERBREAD )
        {
            InputDevice device = InputDevice.getDevice( id );
            return device != null;
        }
        else
        {
            return true;
        }
    }
    
    /**
     * Gets the human-readable name of the hardware.
     * 
     * @param id The unique hardware identifier.
     * 
     * @return The name of the hardware, or null if hardware not found.
     */
    @TargetApi( 9 )
    public static String getHardwareName( int id )
    {
        if( id > HARDWARE_ID_MOGA_OFFSET && id <= HARDWARE_ID_MOGA_MAX )
        {
            return "moga-" + ( id - HARDWARE_ID_MOGA_OFFSET );
        }
        else if( AndroidDevice.IS_GINGERBREAD )
        {
            InputDevice device = InputDevice.getDevice( id );
            return device == null ? null : device.getName();
        }
        else
        {
            return null;
        }
    }
    
    /**
     * Gets the human-readable name of the input.
     * 
     * @param inputCode The universal input code.
     * 
     * @return The name of the input.
     */
    @SuppressWarnings( "deprecation" )
    @TargetApi( 12 )
    public static String getInputName( int inputCode )
    {
        if( inputCode > 0 )
        {
            /*if( AndroidDevice.IS_HONEYCOMB_MR1 )
            {
                String name = null;
                if( inputCode != -1 && AndroidDevice.IS_OUYA_HARDWARE )
                {
                    if( inputCode == OuyaController.BUTTON_A )
                        name = "OUYA BUTTON_A";
                    else if( inputCode == OuyaController.BUTTON_DPAD_DOWN )
                        name = "OUYA BUTTON_DPAD_DOWN";
                    else if( inputCode == OuyaController.BUTTON_DPAD_LEFT )
                        name = "OUYA BUTTON_DPAD_LEFT";
                    else if( inputCode == OuyaController.BUTTON_DPAD_RIGHT )
                        name = "OUYA BUTTON_DPAD_RIGHT";
                    else if( inputCode == OuyaController.BUTTON_DPAD_UP )
                        name = "OUYA BUTTON_DPAD_UP";
                    else if( inputCode == OuyaController.BUTTON_L1 )
                        name = "OUYA BUTTON_L1";
                    else if( inputCode == OuyaController.BUTTON_L2 )
                        name = "OUYA BUTTON_L2";
                    else if( inputCode == OuyaController.BUTTON_L3 )
                        name = "OUYA BUTTON_L3";
                    else if( inputCode == OuyaController.BUTTON_MENU )
                        name = "OUYA BUTTON_MENU";
                    else if( inputCode == OuyaController.BUTTON_O )
                        name = "OUYA BUTTON_O";
                    else if( inputCode == OuyaController.BUTTON_R1 )
                        name = "OUYA BUTTON_R1";
                    else if( inputCode == OuyaController.BUTTON_R2 )
                        name = "OUYA BUTTON_R2";
                    else if( inputCode == OuyaController.BUTTON_R3 )
                        name = "OUYA BUTTON_R3";
                    else if( inputCode == OuyaController.BUTTON_U )
                        name = "OUYA BUTTON_U";
                    else if( inputCode == OuyaController.BUTTON_Y )
                        name = "OUYA BUTTON_Y";
                }
                if( name == null )
                    return KeyEvent.keyCodeToString( inputCode );
                else
                    return name + " (" + KeyEvent.keyCodeToString( inputCode ) + ")";
            }
            else*/
                return "KEYCODE_" + inputCode;
        }
        else if( inputCode < 0 )
        {
            int axis = inputToAxisCode( inputCode );
            String direction = inputToAxisDirection( inputCode ) ? " (+)" : " (-)";
            /*if( AndroidDevice.IS_HONEYCOMB_MR1 )
            {
                String name = null;
                if( axis != -1 && AndroidDevice.IS_OUYA_HARDWARE )
                {
                    if( axis == OuyaController.AXIS_L2 )
                        name = "OUYA AXIS_L2";
                    else if( axis == OuyaController.AXIS_LS_X )
                        name = "OUYA AXIS_LS_X";
                    else if( axis == OuyaController.AXIS_LS_Y )
                        name = "OUYA AXIS_LS_Y";
                    else if( axis == OuyaController.AXIS_R2 )
                        name = "OUYA AXIS_R2";
                    else if( axis == OuyaController.AXIS_RS_X )
                        name = "OUYA AXIS_RS_X";
                    else if( axis == OuyaController.AXIS_RS_Y )
                        name = "OUYA AXIS_RS_Y";
                }
                if( name == null )
                    return MotionEvent.axisToString( axis ) + direction;
                else
                    return name + " (" + MotionEvent.axisToString( axis ) + ")" + direction;
            }
            else*/
                return "AXIS_" + axis + direction;
        }
        else
            return "NULL";
    }
    
    /**
     * Gets the human-readable name of the input, appended with strength information.
     * 
     * @param inputCode The universal input code.
     * @param strength  The input strength, between 0 and 1, inclusive.
     * 
     * @return The name of the input.
     */
    public static String getInputName( int inputCode, float strength )
    {
        return getInputName( inputCode ) + ( inputCode == 0
                ? ""
                : String.format( java.util.Locale.US, " %4.2f", strength ) );
    }    
    
    /**
     * Utility for child classes. Converts an Android axis code to a universal input code.
     * 
     * @param axisCode          The Android axis code.
     * @param positiveDirection Set true for positive Android axis, false for negative Android axis.
     * 
     * @return The corresponding universal input code.
     */
    protected static int axisToInputCode( int axisCode, boolean positiveDirection )
    {
        // Axis codes are encoded to negative values (versus buttons which are positive). Axis codes
        // are bit shifted by one so that the lowest bit can encode axis direction.
        return -( ( axisCode ) * 2 + ( positiveDirection ? 1 : 2 ) );
    }
    
    /**
     * Utility for child classes. Converts a universal input code to an Android axis code.
     * 
     * @param inputCode The universal input code.
     * 
     * @return The corresponding Android axis code.
     */
    protected static int inputToAxisCode( int inputCode )
    {
        return ( -inputCode - 1 ) / 2;
    }
    
    /**
     * Utility for child classes. Converts a universal input code to an Android axis direction.
     * 
     * @param inputCode The universal input code.
     * 
     * @return True if the input code represents positive Android axis direction, false otherwise.
     */
    protected static boolean inputToAxisDirection( int inputCode )
    {
        return ( ( -inputCode ) % 2 ) == 1;
    }
    
    /**
     * Notifies listeners that a single input was dispatched. Subclasses should invoke this method
     * to publish their input data.
     * 
     * @param inputCode  The universal input code that was dispatched.
     * @param strength   The input strength, between 0 and 1, inclusive.
     * @param hardwareId The identifier of the source device.
     */
    protected void notifyListeners( int inputCode, float strength, int hardwareId )
    {
        for( OnInputListener listener : mPublisher )
        {
            listener.onInput( inputCode, strength, hardwareId );
        }
    }
    
    /**
     * Notifies listeners that multiple inputs were dispatched simultaneously. Subclasses should
     * invoke this method to publish their input data.
     * 
     * @param inputCodes The universal input codes that were dispatched.
     * @param strengths  The input strengths, between 0 and 1, inclusive.
     * @param hardwareId The identifier of the source device.
     */
    protected void notifyListeners( int[] inputCodes, float[] strengths, int hardwareId )
    {
        for( OnInputListener listener : mPublisher )
        {
            listener.onInput( inputCodes.clone(), strengths.clone(), hardwareId );
        }
    }
}