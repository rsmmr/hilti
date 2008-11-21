
.. include:: global.inc

Code Generation
===============

Type
    StorageTypes
        Atomic (Non-mutable / Fixed-size / Can live on the stack)
            AtomicSimple
                void
                uint8 uint16 uint32 uint64
                int8  int16  int32  int64
                bool
                double
                time
                interval
      
            ref <Composite>    Reference to *garbage collected* object or NULL
          
      Composite (Mutable / Garbage Collected / Cannot live on the stack)
          string
          struct  { atomic1, atomic2, ... }
          list    <Atomic>
          vector  <Atomic>
          map     <Atomic, Atomic>
          channel <AtomicSimple>
          timer   <Time> <Function> <Args>
          trigger <Function> <Args> ???
          closure <Location> <Frame>
          
    InternalTypes
        TypeDecl
            StructTypeDel
        
        FunctionType
    
        TupleType 

        LableType

        AnyType
          

