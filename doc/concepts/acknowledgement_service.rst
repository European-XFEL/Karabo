.. _acknowledgement_service:

************************
Acknowledgement Messages
************************

Acknowledgement messages are messages which block program flow on a device until they 
are either acknowledged or declined. The messages appear on a special acknowledgement
device, the id of which is configurable for each acknowledgment call, but defaults
to a standard base device provided instance. Interaction with these devices is via their
*slotAcknowledge* slot:

.. code-block:: Python

    def aCallToBeAcknowledged(self):
    	response = bool()
    	message = "Acknowledge me please!"
        self._ss.request("acknowledgeDeviceId", "slotAcknowledge", message)
                         .timeout(-1).reply(response)
        if response == True:
            ...
            #do whatever is to be done for positive acknowledgement
        else:
            ...
            #request was declined, do something different.
            
    
The *acknowledgment* device will block the request until an action by the user has been
taken. You should thus make sure that whenever you request an acknowledgement you are
at a point in code which may block for a long time. Another option is to set a timeout
on the request and catch the resulting timeout-error if an acknowledgement does not occur
by then.

.. note::

	You should take care not to misuse the acknowledgement service for a lot of 'are you sure'
	type messages. Rather than relying on a human to take the right action you should
	catch improper inputs and act accordingly in a programmatic fashion.
	
.. note::

	A good use for the acknowledgment service is to ask for an acknowledgement on manual
	override commands.

.. todo::

	Decide on how to implement this functionality as an acknowledgement device. I think
	it is possible with existing functionality.
	

Requesting Control over Shared Resources - Client Locking
---------------------------------------------------------

A recurring scenario for acknowledgements is to request control over a shared resource.
This is the case, for example, for the pump-probe lasers shared amongst two hutches. In such a
scenario acknowledgments are used in conjunction with *client-locking*. The procedure
is as follows:

1. You request the acknowledgment through a share acknowledgment service
2. After positive acknowledgment you set the *topicsAllowedToSet* property to a
   string list of hostname which you have access to the device.
3. Clients running on a topic now not in this list will refuse executing property
   assignments or commands. The GUI will show the device in the interlocked state.
   
.. warning::

  To allow all clients with proper access rights from interacting with the device again
  the *topicsAllowedToSet* property should be set to an empty string.
  
.. todo::

	Discuss how this concept can be implemented. Burkhard, Kerstin, Gero
	This functionality should probably be implemented as a generic middle-layer device
	which takes a list of device id and clients they should be enabled for.
	
.. ifconfig:: includeDevInfo is True

    Client-side locking is implemented on a per-topic level. For devices
    communicating with multiple broker topics, all but one topic may selectively
    be disabled for assignment and command communications. The device is then
    locked for these topics.

    .. todo::

        Implement client-side locking.
	
