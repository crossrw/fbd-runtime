fbd-free
========

run-time FBD library for PLC (Programmable logic controller).

About FBD: http://en.wikipedia.org/wiki/Function_block_diagram

FBD editor can be downloaded at https://dl.dropboxusercontent.com/u/46913329/fbd2.zip

========

1. Appointment
Language functional diagrams designed to describe the functioning of the single logic controller or multiple controllers connected information network. Language describes a scheme consisting of the elements and their binding chains.

2. Basic Concepts

The scheme is a set of elements and relationships between them.

  Circuit element is the minimum functional circuit block that implements a specific operation (logic, arithmetic, delay, etc.) or provide connectivity scheme with input or output hardware controller circuits. Each element may be greater than or equal to zero the number of signal inputs and zero or one output signal. Inputs are numbered element - a number from 0 to N-1 , where N - number of inputs. Number of inputs and outputs depends on the type of item. For each input item, usually must be connected an external circuit. Inputs of some elements may have value "default". Value of the input to which no external circuit and there is no "default" is undefined, and the circuit in which there is an element, incorrect. Furthermore, the element may be greater than or equal to zero quantity affecting its functioning named parameters. Fixed values ​​and are given in the design scheme. Each element must have a unique (within the scheme ), a name that will identify it. Elements that do not contain the output, usually serve to anchor chains scheme to hardware outputs of the controller or the formation of values, accessible through the data interface controller. Binding is defined by the parameters of the element. Elements that do not contain entries to serve as a source of fixed or alternating signal, whose value is given by a parameter. In addition, setting the value of the signal values of pixels may be accessed via an information controller interface.

  Chain is a logical connection between input and output elements and serves to transmit the signal states . Each circuit must be connected strictly one output element. State of the chain which is not connected to any output or connected more than one output of several elements is undefined , and a scheme in which to make such connections , incorrect. Chain, which is not connected to any input element is unused and its value is not calculated. Each chain has a unique (within the scheme) name. Chain with the same name are one chain.

  Chains are carriers of signals. The signal is expressed in a language in the form of integer values with a sign. In general, the signal is used to represent the four-byte number. Due to the nature of hardware controllers can be used at the bit number. To perform logical operations, the logical "0" is defined as the signal value "<= 0", a logical "1" - signal value ">=1". Furthermore, the modifications of the signal value of the logical state "0" to logic "1" (rising edge), and from a logical "1" to logical "0" (a falling edge).

3. Algorithm for calculating the state of the circuit

Calculation of the state controller scheme executed cyclically with a certain period.
Each cycle calculation circuit state is to calculate output signals to the output elements which are not connected to the circuit.
Payment scheme in the following sequence:
- for all circuits schemes establish a criterion of "no data";
- scheme of all content elements to the output circuit which is not connected;
- calculate the output value of the element:
  - Calculated value input element: if the chain is set sign "unknown", the calculated output value is connected to the circuit element, otherwise already taken the calculated value chain;
  - if in the process of circuit design need her own value (feedback loop), then used for the calculation of its previous value;
  - the value chain which has already been calculated, a sign of "no data" is reset;
  - the operation is performed on the input signal element (depending on the type of item).
The operation is performed for all selected items.


to be continued ...
