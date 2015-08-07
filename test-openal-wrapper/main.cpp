#include <oalplus/al.hpp>
#include <oalplus/all.hpp>
#include <oalplus/alut.hpp>
#include <chrono>
#include <thread>

oalplus::ALUtilityToolkit* alut{};
class Sound
{
    public:
        Sound():
            m_source{oalplus::ObjectDesc("ASource")}
        {
            // create a Hello World sound and store it into a buffer
            oalplus::Buffer buffer = alut->CreateBufferHelloWorld();

            // create a source from the data in buffer and set enable looping
            m_source = oalplus::Source(oalplus::ObjectDesc("Source"));
            m_source.Buffer(buffer);
            m_source.Looping(true);
            m_source.ReferenceDistance(15);
        }

        auto& source()
        { return m_source; }

        void setEnabled(bool)
        {

        }

    private:
        oalplus::Source m_source;
};

class Scene
{
    public:
        Scene()
        {
            // create a listener and set its position, velocity and orientation
            listener.Position(0.0f, 0.0f, 0.0f);
            listener.Velocity(0.0f, 0.0f, 0.0f);
            listener.Orientation(0.0f, 0.0f,-1.0f, 0.0f, 1.0f, 0.0f);

            m_sounds.resize(1);

        }

        void start()
        {
            for(auto& sound : m_sounds)
            {
                auto& source = sound.source();
                // let the source play the sound
                source.Play();

                // the path of the source
                oalplus::CubicBezierLoop<oalplus::Vec3f, double> path({
                                                                          oalplus::Vec3f( 0.0f, 0.0f, -9.0f),
                                                                          oalplus::Vec3f( 9.0f, 2.0f, -8.0f),
                                                                          oalplus::Vec3f( 4.0f,-3.0f, 9.0f),
                                                                          oalplus::Vec3f(-3.0f, 4.0f, 9.0f),
                                                                          oalplus::Vec3f(-9.0f,-1.0f, -7.0f)
                                                                      });
                //
                // play the sound for a while
                typedef std::chrono::system_clock clock;
                typedef std::chrono::time_point<clock> time_point;
                time_point start = clock::now();
                while(true)
                {
                    double t = double((clock::now() - start).count())*
                               double(clock::period::num)/
                               double(clock::period::den);
                    if(t > 10.0) break;
                    source.Position(path.Position(t/5.0));
                    listener.Position(listener.Position() - oalplus::Vec3f{0.2, 0, 0});
                    // wait for a while
                    std::chrono::milliseconds duration(10);
                    std::this_thread::sleep_for(duration);
                }
            }
        }

        auto& listener()
        { return m_listener; }

    private:
        oalplus::Listener listener;
        std::vector<Sound> m_sounds;
};

int main(int argc, char** argv)
{
    // open the default device
    oalplus::Device device;
    // create a context using the device and make it current
    oalplus::ContextMadeCurrent context(
                device,
                oalplus::ContextAttribs()
                .Add(oalplus::ContextAttrib::MonoSources, 1)
                .Get()
                );
    // create an instance of ALUT
    alut = new oalplus::ALUtilityToolkit(false, argc, argv);

    Scene s;
    s.start();

    //
    return 0;
}
