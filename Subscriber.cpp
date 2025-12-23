#include <ace/Log_Msg.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include "MessageTypeSupportImpl.h"
#include <iostream>
#include <thread>

// 1. Definimos la clase "Listener" (El Manejador de Eventos)
// Hereda de una clase base de DDS para poder sobreescribir sus métodos
class MiListener : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  // Este método se ejecuta AUTOMÁTICAMENTE cuando llega un dato nuevo
  virtual void on_data_available(DDS::DataReader_ptr reader) {
    try {
      // 1. Convertir el "Reader genérico" a nuestro "Reader de Posicion"
      Demo::PosicionDataReader_var reader_i =
        Demo::PosicionDataReader::_narrow(reader);

      if (!reader_i) {
        std::cerr << "Error en narrow del reader" << std::endl;
        return;
      }

      // 2. Intentar sacar (take) el mensaje de la cola
      Demo::Posicion mensaje;
      DDS::SampleInfo info;

      DDS::ReturnCode_t error = reader_i->take_next_sample(mensaje, info);

      if (error == DDS::RETCODE_OK) {
        // Verificar si es un dato válido (y no un aviso de desconexión)
        if (info.valid_data) {
          std::cout << "recibido: ID=" << mensaje.id 
                    << " | Texto='" << mensaje.texto 
                    << "' | Contador=" << mensaje.contador << std::endl;
        }
      }

    } catch (const CORBA::Exception& e) {
      e._tao_print_exception("Excepción en on_data_available: ");
    }
  }

  // Estos métodos son obligatorios por la interfaz, pero los dejamos vacíos por ahora
  virtual void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&) {}
  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&) {}
  virtual void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&) {}
  virtual void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&) {}
  virtual void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&) {}
  virtual void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {}
};

int main(int argc, char* argv[]) {
  try {
    // 2. Inicialización estándar (Idéntica al Publisher)
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Conectarse al MISMO dominio (ID 42)
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      std::cerr << "Error creando participant" << std::endl;
      return 1;
    }

    // Registrar el tipo de dato
    Demo::PosicionTypeSupport_var ts = new Demo::PosicionTypeSupportImpl;
    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
      std::cerr << "Error registrando tipo" << std::endl;
      return 1;
    }

    // Crear el Tópico "PosicionTopic"
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("PosicionTopic",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // 3. Crear el Subscriber
    DDS::Subscriber_var subscriber =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // 4. Instanciar nuestro Listener y el DataReader
    DDS::DataReaderListener_var listener(new MiListener);

    // Creamos el DataReader y le ASIGNAMOS el listener (y una máscara para avisar que escuche todo)
    DDS::DataReader_var reader =
      subscriber->create_datareader(topic,
                                    DATAREADER_QOS_DEFAULT,
                                    listener,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    std::cout << "Esperando mensajes (Ctrl+C para salir)..." << std::endl;

    // 5. Mantener el programa vivo
    // Como el listener funciona en otro hilo, el main solo tiene que evitar cerrarse.
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Excepción en main: ");
    return 1;
  }
  return 0;
}