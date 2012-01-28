.. Automatically generated. Do not edit.

================================   ============================================================================   ========================================  
HILTI Type                         C Type                                                                         Header                                    
================================   ============================================================================   ========================================  
:hlt:type:`Label`                  void *                                                                                                                   
:hlt:type:`addr`                   `hlt_addr <doxygen/struct____hlt__addr.html>`_                                 `addr.h <doxygen/group__addr.html>`_      
:hlt:type:`any`                    void *                                                                                                                   
:hlt:type:`bitset`                 uint64_t                                                                       `bitset.h <doxygen/group__bitset.html>`_  
:hlt:type:`bool`                   int8_t                                                                                                                   
:hlt:type:`bytes`                  `hlt_bytes * <doxygen/struct____hlt__bytes.html>`_                                                                       
:hlt:type:`caddr`                  void *                                                                                                                   
:hlt:type:`callable`               `hlt_callable * <doxygen/struct____hlt__callable.html>`_ [#hlttypecallable]_                                             
:hlt:type:`channel`                hlt::channel *                                                                                                           
:hlt:type:`classifier`             `hlt_classifier * <doxygen/struct____hlt__classifier.html>`_                                                             
:hlt:type:`double`                 double                                                                                                                   
:hlt:type:`enum`                   `hlt_enum <doxygen/struct____hlt__enum.html>`_                                                                           
:hlt:type:`exception`              `hlt_exception * <doxygen/struct____hlt__exception.html>`_                                                               
:hlt:type:`file`                   `hlt_file * <doxygen/struct____hlt__file.html>`_                                                                         
:hlt:type:`int`                    int<n>_t                                                                                                                 
:hlt:type:`interval`               `hlt_interval <doxygen/struct____hlt__interval.html>`_ [#hlttypeinterval]_                                               
:hlt:type:`iosrc`                  `hlt_iosrc * <doxygen/struct____hlt__iosrc.html>`_                                                                       
:hlt:type:`list`                   `hlt_list * <doxygen/struct____hlt__list.html>`_                                                                         
:hlt:type:`map`                    `hlt_map * <doxygen/struct____hlt__map.html>`_                                                                           
:hlt:type:`match_token_state`      void *                                                                                                                   
:hlt:type:`net`                    `hlt_net <doxygen/struct____hlt__net.html>`_                                                                             
:hlt:type:`overlay`                `hlt_overlay <doxygen/struct____hlt__overlay.html>`_                                                                     
:hlt:type:`port`                   `hlt_port <doxygen/struct____hlt__port.html>`_                                                                           
:hlt:type:`ref`                    void *                                                                                                                   
:hlt:type:`regexp`                 `hlt_regexp * <doxygen/struct____hlt__regexp.html>`_                                                                     
:hlt:type:`set`                    `hlt_set * <doxygen/struct____hlt__set.html>`_                                                                           
:hlt:type:`string`                 `hlt_string <doxygen/struct____hlt__string.html>`_                                                                       
:hlt:type:`time`                   `hlt_time <doxygen/struct____hlt__time.html>`_ [#hlttypetime]_                                                           
:hlt:type:`timer`                  `hlt_timer * <doxygen/struct____hlt__timer.html>`_                                                                       
:hlt:type:`timer_mgr`              `hlt_timer_mgr * <doxygen/struct____hlt__timer__mgr.html>`_                                                              
:hlt:type:`vector`                 `hlt_vector * <doxygen/struct____hlt__vector.html>`_                                                                     
iterator<:hlt:type:`bytes`>        `hlt_bytes_pos <doxygen/struct____hlt__bytes__pos.html>`_                                                                
iterator<:hlt:type:`iosrc`\<K>>    `hlt_iosrc* <doxygen/struct____hlt__iosrc*.html>`_                                                                       
iterator<:hlt:type:`list`\<T>>     `hlt_list_iter <doxygen/struct____hlt__list__iter.html>`_                                                                
iterator<:hlt:type:`map`\<K,V>>    `hlt_map_iter <doxygen/struct____hlt__map__iter.html>`_                                                                  
iterator<:hlt:type:`set`\<T>>      `hlt_set_iter <doxygen/struct____hlt__set__iter.html>`_                                                                  
iterator<:hlt:type:`vector`\<T>>   `hlt_vector_iter <doxygen/struct____hlt__vector__iter.html>`_                                                            
================================   ============================================================================   ========================================  

.. rubric:: Notes

.. [#hlttypeany]
   Parameters are always passed with type information.

.. [#hlttypebool]
   ``True`` corresponds to the value ``1`` and ``False`` to value
   ``0``.

.. [#hlttypecallable]
   A ``callable`` object is mapped to ``hlt_callable *, where
   ``hlt_callable`` is itself a pointer. This double indirection allows
   us to track when a continuation has been called by setting the outer
   pointer to null.``.

.. [#hlttypeint]
   An ``int<n>`` is mapped to C integers depending on its width *n*,
   per the following table:
   
   ======  =======
   Width   C type
   ------  -------
   1..8    int8_t
   9..16   int16_t
   17..32  int32_t
   33..64  int64_t
   ======  =======

.. [#hlttypeinterval]
   The 32 most-sigificant bits are representing seconds; the 32-bit
   least-signifant bits nanoseconds.

.. [#hlttyperef]
   A ``ref<T>`` is mapped to the same type as ``T``. Note that because
   ``T`` must be a ~~HeapType, and all ~~HeapTypes are mapped to
   pointers, ``ref<T>`` will likewise be mapped to a pointer. In
   addition, the type information for ``ref<T>`` will be the type
   information of ``T``. Values of type wildcard reference ``ref<*>``
   will always be passed with type information.

.. [#hlttypetime]
   The 32 most-sigificant bits are representing seconds; the 32-bit
   least-signifant bits nanoseconds.

