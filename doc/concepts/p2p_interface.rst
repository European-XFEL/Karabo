.. _p2p:

**********************************
The Point-to-Point (p2p) Interface
**********************************

.. sectionauthor:: Gero Flucke <gero.flucke@xfel.eu>

The conveyor belt example given in the introduction of the C++ and Python API's is
typical for a device that represents some hardware that is controlled and monitored.
No data is *produced* by these devices, but just property changes are communicated.
This is done via the central message broker.

Other devices like cameras produce large amounts of data that cannot be
distributed this way. Instead, the data should be sent directly from one device
producing it to one or more other devices that have registered themselves
at the producer for that purpose. In the Karabo framework this can be done
using a point-to-point protocol between so called output and input channels
of devices.

Sending Data: Output Channel
----------------------------
First, we have to tell the framework what data is to be sent via the output
channel, i.e. to declare its schema.
This is done inside the ``expectedParameters`` method.
Here is an example of a device sending a 32-bit integer, a string and
a vector of 64-bit integers:

.. code-block:: c++

        Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("string")
                .readOnly()
                .commit();

        VECTOR_INT64_ELEMENT(data).key("vector_int64")
                .readOnly()
                .commit();

Next (but still within ``expectedParameters``), the output channel has to be
declared. Here we create one with the key *output*:

.. code-block:: c++

        OUTPUT_CHANNEL(expected).key("output")
                .displayedName("Output")
                .dataSchema(data)
                .commit();

Whenever the device should write data to this output channel,
a ``Hash`` should be created that matches this schema

.. code-block:: c++

        Hash data;
        data.set("dataId", 5);
        data.set("string", std::string("This is a string to be sent."));
        std::vector<long long> vec = ...; // create and fill the array here
        data.set("vector_int64", vec);

Note that Karabo does not check that the data sent matches the
declared schema.

Finally, the data is sent by calling the device method

.. code-block:: c++

        this->writeChannel("output", data);

with the key of the channel as the first and the ``Data`` object as the second
argument.

Once the data stream is finished, i.e. no further data is to be sent, the
end of stream method has to be called with the output channel key as argument
to inform all input channels that receive the data:

.. code-block:: c++

        this->signalEndOfStream("output");


Receiving Data: Input Channel
-----------------------------
For input channels one also needs to declare what data they expect to receive.
This is done in exactly the same way as for output channels inside the
``expectedParameters`` method.
Declaring the input channel is also analogue to the way an output channels is
declared:

.. code-block:: c++

        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .description("Input channel: client") // optional, for GUI
                .dataSchema(data)
                .commit();

The next step is to prepare a member function of the device that should be
called whenever new data arrives. The signature of that function has to be

.. code-block:: c++

   void onData(const karabo::util::Hash& data,
               const karabo::xms::InputChannel::MetaData& meta);


Inside the function the data sent can be unpacked from the Hash:

.. code-block:: c++

   int id = data.get<int>("dataId");
   const std::string& str = data.get<std::string>("string");
   const vector<long long>& vec = data.get<std::vector<long long> >("vector_int64");


Finally, the framework has to be informed that this method should be called
whenever data arrives. This has to be done in the ``initialize()`` member
function (or, more precisely, in the function registered in the constructor
using the ``KARABO_INITIAL_FUNCTION`` macro) in the following way:

.. code-block:: c++

   KARABO_ON_DATA("input", onData);

with the key of the input channel as first and the function name as the second
argument.

A similar macro can be used to register a member function that should be called
when the data stream terminates, i.e. when the sending device calls
``this->signalEndOfStream("<output channel name>");``:

.. code-block:: c++

  KARABO_ON_EOS("input", onEndOfStream);

The signature of this member function has to be

.. code-block:: c++

   void onEndOfStream(const karabo::xms::InputChannel::Pointer& input);


.. note::

    A simple way of ensuring that input and output channels work with the
    same data schema is to move schema creation to a static function which
    is availble to all devices working on this type of data, e.g. by means
    of a dependency or library.

Hierarchies in the Schema
-------------------------

The data that is sent from an output to an input channel can have a hierarchical
structure. This structure is declared in the usual way in
``expectedParameters``, for both input and output channels:

.. code-block:: c++

        Schema data;
        // Add whatever data on first hierarchy level:
        // ...
        // First level done - now add second level:
        NODE_ELEMENT(data).key("node")
                .commit();

        FLOAT_ELEMENT(data).key("node.afloat")
                .readOnly()
                .commit();

When writing to an output channel, one first has to create and fill the node.
Then the node can be added and the data can be sent:

.. code-block:: c++

        Hash data; // top level data structure
        // Here e.g. fill top level content:
        // ...
        Hash node;
        float floatValue = 1.3f;  // or whatever...
        node.set("afloat", floatValue);
        data.set("node", node);
        this->writeChannel("output", data);

In the ``onData`` member function of a device receiving the data in an input
channel, the node can be unpacked in the following way:

.. code-block:: c++

    void onData(const karabo::xms::Data& data,
                const karabo::xms::InputChannel::MetaData& meta);
    {
      // ...
      Hash node(data.get<Hash>("node"));
      const float afloat = node.get<float>("afloat");
      // ...
    }


Treatment of Array Data
-----------------------

Arrays are described in Karabo using the ``NDArray`` class.
 An ``NDArray`` consists of typed data and a shape.
It is meant to map directly to a ``numpy.ndarray`` object in the Bound API,
so its interface closely matches ``numpy.ndarray``.

.. code-block:: c++

        NDARRAY_ELEMENT(expected).key("arrayStack")
                .shape("-1,100,100") // Variable dimension along the slowest axis
                .readOnly().noInitialValue()
                .commit();


In the above example ``-1`` in the shape definition indicates a variable size of
this dimension; e.g. the first dimension is of variable size. If the shape
contains no negative numbers, the array is said to have a 'fixed' shape.

In Python, a transparent conversion to and from ``numpy.ndarray`` elements
is performed:

.. code-block:: python

    a = np.ones((10, 100, 100))
    self.set("arrayStack", a)
    b = self.get("arrayStack")
    type(b)
    >>> numpy.ndarray

    c = np.ones((10, 10, 100))
    self.set("arrayStack", c)
    >>> ValueError("Setting 'arrayStack' failed because dimension 2 in \
    (10, 10, 100) mismatched array shape definition (-1, 100, 100)")


The ``NDArray`` C++ class is a convenience class meant to
simplify supporting n-dimensional arrays within the ``Device`` and ``Hash``
classes. In C++ the ``Device::set`` method is overwritten to accept ``NDArray``
objects directly:

.. code-block:: C++

        typedef std::vector<double> DoubleVector;
        typedef boost::shared_ptr<DoubleVector> DoubleVectorPtr;
        DoubleVectorPtr v(new DoubleVector(10*100*100, 1));
        NDArray<double> arr(v, Dims(10, 100, 100));
        set("arrayStack", arr);
        // ... Then access the array
        NDArray a = get<NDArray>("arrayStack");
        const Dims & d = a.getDims();
        DoubleVectorPtr v1 = a.getData();

Using the above constructor no copy of the data is performed. Alternatively,
the a copying constructor may be used

.. code-block:: C++

        typedef std::vector<double> DoubleVector;
        typedef boost::shared_ptr<DoubleVector> DoubleVectorPtr;
        DoubleVector v(10*100*100, 1);
        NDArray arr(v, Dims(10, 100, 100));
        set("arrayStack", arr);
        // ... Then access the array
        NDArray<double> a = get<NDArray >("arrayStack");
        const Dims & d = a.getDims();
        DoubleVectorPtr v1 = a.getData();

In this case ``NDArray`` will create a copy of the data, but internally also
maintains it as a ``boost::shared_ptr``, thus avoiding additional copies from
there on. In either case access to the data is via a ``boost::shared_ptr``
using ``getData()``.

Internally, ``NDArray`` uses a ``ByteArray`` to hold its data, while additionally,
defining the shape as an attribute in a standardized fashion. ``NDArray``s
can be placed and retrieved from Hashes in the accustomed way:


Treatment of Image Data
-----------------------
As with array data, image data can similarly be sent using the class
``ImageData`` which extends on-top of the ``NDArray`` class with some predefined properties,
i.e. it serves as a special node with convenience methods for conversions to
and from more useful image data formats. The schema of an output channel for
image data is defined in ``expectedParameters`` as follows:

.. code-block:: c++

        Schema data;
        IMAGEDATA(data).key("image")
                .encodingType(karabo::xms::Encoding::RGBA)
                .bitsPerPixel(12)
                .isBigEndian(true)
                .commit();

        OUTPUT_CHANNEL(expected).key("output") // or any other key
                .displayedName("Output")       // or whatever name you choose
                .dataSchema(data)
                .commit();

For input channels simply replace ``OUTPUT_CHANNEL`` by ``INPUT_CHANNEL``.

Image data refers to array-like data from camera interfaces. It may be
represented as an ``IMAGEDATA``(or ``IMAGEDATA_ELEMENT``) element, which
has fixed properties approriate to the camera origin of the data. These are:

- pixels: he N-dimensional array containing the pixels
- dims: The length of the array reflects total dimensionality and each element the extension in this dimension
- dimTypes: Any dimension should have an enumerated type
- dimScales: Dimension Scales
- encoding: Describes the color space of pixel encoding of the data (e.g. GRAY, RGB, JPG, PNG etc
- bitsPerPixel: The number of bits needed for each pixel
- roiOffsets: Describes the offset of the Region-of-Interest; it will contain zeros if the image has no ROI defined
- geometry: optional hierarchical detector geometry information
- header: Hash containing user-defined header data


Interface *per TCP Message*
---------------------------

Point-to-point communication in the Karabo framework generally uses TCP for
data transfer between devices.
Whenever ``writeChannel`` is called for an output channel, the data is sent as
a separate message to all connected input channels.
There might be circumstances where it is advantageous to pack more than one
data item into a TCP message. For this a lower level API is provided as
described in the following.

To sent several data items in a single TCP message, the following few lines
of code should be used instead of ``this->writeChannel(channelName, data)``:

.. code-block:: c++

    data.attachTimestamp(this->getActualTimestamp());
    karabo::xms::OutputChannel::Pointer channel = this->getOutputChannel(channelName);
    channel->write(data);

Once there is enough data accumulated to be actually sent,

.. code-block:: c++

    channel->update();

has to be called.

For a device with an input channel it does not matter much whether several
data items that it receives have been sent in a single TCP message or not.
A member function registered with ``KARABO_ON_DATA`` will be called
for each item. Nevertheless, in case it matters which data items are sent
together (which should not be the case), the device can register a method
that receives all data items in one go.
Instead of using ``KARABO_ON_DATA``, such a method has to be registered
using ``KARABO_ON_INPUT``. The signature of this method has to be

.. code-block:: c++

   void onInput(const karabo::xms::InputChannel::Pointer& input);


Inside the method one has to loop over the data items. Finally one has to
tell the ``InputChannel`` that reading the data is done by calling
``update()`` at the very end of the method:

.. code-block:: c++

       for (size_t i = 0; i < input->size(); ++i) {
            Hash data(input->read(i));
            ... // whatever you want to do with the data
        }
        // Tell the input channel that you are done with all data
        input->update();


Compliance with Data Management
-------------------------------

While the pipeline processing interface generally allows free form Hashes to
be passed between devices, leaving it up to the device logic to correctly
interpret these, there are limitations if data is be written to or retrieved
from the data management system. Specifically, Hashes need to follow a certain
structure, and the concept of meta-data needs to be understood.


Meta Data
=========

So far we have simply written to output channels and ignored the fact that
each data token written has meta data pertinent to it. This meta data currently
contains source and timing information, but is by design extensible. If not
explicitly set, the source corresponds to the writing device's id and the
output channel name, and the timing information to the train and timestamp
for when the data was written. Frequently, source information should be maintained
though, i.e. the writing device is *not* the data producer. In this case
we explicitly set the source or forward existing meta data:

.. code-block:: c++

   using namespace karabo::xms;
   OutputChannel::Pointer channel = this->getOutputChannel(channelName);

   for (size_t i = 0; i < input->size(); ++i) {
        Hash data;
        const InputChannel::MetaData& meta = (input->read(i, data));
        ... // whatever you want to do with the data
        channel->write(data, meta);

        const InputChannel::MetaData& meta2;
        meta2.setSource("myProcessor");
        channel->write(data, meta2);

    }
    // Tell the input channel that you are done with all data
    input->update();

Metadata can be accessed either via ``read`` or by indices. Data tokens for
the same source can be written subsequently to the same output channel, allowing
e.g. to bunch multiple trains before actually writing data out to the network.

.. warning::

    The data management service expects only one train per data token per source
    to be sent to it.

In all cases the source information will be used by the data management system
to correlate incoming data with data producers.

Hash Structure
==============

For data interacting with the data management system and additional restriction
applies in terms of Hash structure. Generally, data of similar types is organized
hierarchical in nodes. The following data types exist:

Train Data
++++++++++

Is data that occurs on a per train basis. It can be in form of scalars, vectors
or arrays. For interaction with the data management system a data token written
to the channel always corresponds to a train. The Hash that is written must
match the following policy:

- on the root hierarchy level an unsigned long long element giving the trainId
  exists
- freely named nodes enclosing exist on the same hierarchy level, which have
  an attribute ``daqDataType`` set to ``TRAIN``. Underneath these nodes scalar
  vector and array elements exist.

Pulse Data
++++++++++

Pulse data is data that has pulse resolution. Data can either exist for all
pulses of a train or a subset. In either case the following limitations
apply:

- the enclosing node element has the ``daqDataType`` set to ``PULSE``.
- a vector unsigned long long element with key  ``pulseId`` needs to be
  located directly underneath this node element
- any number of freely vector elements with the same length as ``pulseId``
  may reside underneath this node, or in further sub nodes. There is a 1:1
  relation between the index in these elements and the pulse id given in ``pulseId``
  at this index.
- any number of freely NDArray elements with the last dimension of the same length
  as ``pulseId`` may reside underneath this node, or in further sub nodes. There is a 1:1
  relation between the index of the last dimension in these elements and the pulse
  id given in ``pulseId`` at this index. The other dimensions may not change
  from data token to data token.

There may be any number of these node elements, all following the above structure.
They may be freely named, except that the key ``masterPulseData`` is reserved.

All train and pulse data elements must always be present in all hashes, even
if the arrays or vectors are empty.

All train and pulse data elements must be specified in the output
channel's data schema. Adding additional elements in between data tokens,
specifically between runs is not allowed.



Defining and Configuring Topologies
===================================

Pipelined processing in Karabo supports a variety of recurring topologies
defining how data is passed throught the system.

Copying vs. Sharing Data
++++++++++++++++++++++++

An input channel may selected if it would like to receive data in *copy* or
*shared* mode. In the first case it will receive a copy of all data sent by
output channel it is connected to. In shared mode, the output channel is allowed
to balance data distribution according to how it is configured. There are
two options on the output channel:

round robin
    distributes data evenly on all connected input channels operating in
    shared mode. As indicated by the option name channels subsequently get
    data send to them. If the next channel in line is not available yet, writes
    to the output channel block until the data can be sent to this channel.

load-balanced
    distributes data on all connected input channels but does not enforce
    a particular distribution order. Upon writing data to an output channel
    it is sent to the next available input channel. This scenario should
    be used if data recipients are expected to have different processing times
    on data packages.

Best-Effort and Assured Delivery Configuration
++++++++++++++++++++++++++++++++++++++++++++++

Both input and output channels may be configured on what to do if the counter
part is not available, i.e. no input is ready to receive data from a given
output. Options are to

throw
    an exception.

queue
    the data and deliver it once an input becomes available. The write call
    to the output channel will not block.

wait
    for an input to become avaible, effectivly blocking the write call to the
    output channel.

drop
    the data being sent out, i.e. do not send it but proceed with-out blocking
    the write call to the output.

.. note::

    Queuing data may involve significant memory usage and thus should be used
    with care if large amounts of data are passed.


By default the channels are configured to *wait* behaviour, which assures delivery
but has the side effect of possibly stalling a complete processing pipeline by
back-propagation. If a pipeline device with an input and output channels is
used as a pipeline end-point, it is important to configure the last, unconnected
output to drop to avoid this scenario from happending.
