#include <ace/Log_Msg.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include "MessageTypeSupportImpl.h" // Generado por IDL
#include <iostream>
#include <thread> // Para simular espera entre mensajes

int main(int argc, char* argv[]) {
  try {
    // 1. Inicializar el Participante (La "Entidad" en el dominio DDS)
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    
    // Conectarse al Dominio 42 (Un numero arbitrario, debe coincidir en ambos lados)
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      std::cerr << "Error creando el participant" << std::endl;
      return 1;
    }

    // 2. Registrar el Tipo de Dato (El "Contrato" IDL)
    Demo::PosicionTypeSupport_var ts = new Demo::PosicionTypeSupportImpl;
    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
      std::cerr << "Error registrando el tipo" << std::endl;
      return 1;
    }

    // 3. Crear el Topic (El "Tema" de conversación)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("PosicionTopic",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // 4. Crear el Publisher (El objeto que gestiona la publicación)
    DDS::Publisher_var publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    0,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // 5. Crear el DataWriter (El que realmente escribe los datos)
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic,
                                   DATAWRITER_QOS_DEFAULT,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Castear el writer genérico a nuestro tipo específico
    Demo::PosicionDataWriter_var pos_writer =
      Demo::PosicionDataWriter::_narrow(writer);

    // 6. Ciclo de Publicación
    Demo::Posicion mensaje;
    mensaje.id = 1; // ID del "Avión" o entidad
    mensaje.contador = 0;

    std::cout << "Empezando a publicar..." << std::endl;

    while (true) {
      mensaje.contador++;
      mensaje.texto = CORBA::string_dup("Mensaje desde WSL Publisher");

      std::cout << "Enviando mensaje #" << mensaje.contador << std::endl;
      
      // ¡Aca sucede la magia! Se envía a la red.
      DDS::ReturnCode_t error = pos_writer->write(mensaje, DDS::HANDLE_NIL);

      if (error != DDS::RETCODE_OK) {
        std::cerr << "Error al enviar datos" << std::endl;
      }

      // Esperar 1 segundo antes del próximo mensaje
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Excepción en main: ");
    return 1;
  }

  return 0;
}