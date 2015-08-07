#include <oalplus/al.hpp>
#include <oalplus/all.hpp>
#include <oalplus/alut.hpp>
#include <chrono>
#include <thread>
#include <iostream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
class Sound
{
    public:
        Sound():
            Sound{oalplus::ALUtilityToolkit(false).CreateBufferHelloWorld()}
        {
        }

        Sound(const std::string& filename):
            Sound{oalplus::ALUtilityToolkit(false).CreateBufferFromFile(filename)}
        {
        }

        Sound(const oalplus::Buffer& buf):
            m_source{oalplus::ObjectDesc("Source")}
        {
            m_source.Buffer(buf);
            m_source.Looping(true);
            m_source.ReferenceDistance(15);
        }

        auto& source()
        {
            return m_source;
        }

        const auto& source() const
        {
            return m_source;
        }

        void setEnabled(bool)
        {

        }

        const std::string& name() const
        {
            return m_name;
        }

        void setName(const std::string &name)
        {
            m_name = name;
        }

    private:
        std::string m_name;
        oalplus::Source m_source;
};

namespace bmi = boost::multi_index;
using SoundMap = bmi::multi_index_container<
    Sound,
    bmi::indexed_by<
        bmi::hashed_unique<
            bmi::const_mem_fun<
                Sound,
                const std::string&,
                &Sound::name
            >
        >
    >
>;
class Scene
{
    public:
        Scene()
        {
            // create a listener and set its position, velocity and orientation
            m_listener.Position(0.0f, 0.0f, 0.0f);
            m_listener.Velocity(0.0f, 0.0f, 0.0f);
            m_listener.Orientation(0.0f, 0.0f,-1.0f, 0.0f, 1.0f, 0.0f);

            m_sounds.insert(Sound());
        }

        void start()
        {
            for(auto& cst_sound : m_sounds)
            {
                auto& sound = const_cast<Sound&>(cst_sound);
                auto& source = sound.source();
                // let the source play the sound
                source.Play();
                /*

                // the path of the source
                oalplus::CubicBezierLoop<oalplus::Vec3f, double> path(
                {
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
                    m_listener.Position(m_listener.Position() - oalplus::Vec3f{0.2, 0, 0});
                    // wait for a while
                    std::chrono::milliseconds duration(10);
                    std::this_thread::sleep_for(duration);
                }
                */
                while(true)
                    std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }

        oalplus::Listener& listener()
        { return m_listener; }

        SoundMap& sounds()
        { return m_sounds; }

    private:
        oalplus::Listener m_listener;
        SoundMap m_sounds;
};

#include <Network/Device.h>
#include <Network/Protocol.h>
#include <Network/Node.h>



template<typename Get, typename Set>
struct Parameter
{
        Get get;
        Set set;
};


template<typename Get, typename Set>
auto make_parameter(Get&& get, Set&& set)
{
    return Parameter<Get, Set>{std::move(get), std::move(set)};
}

template<typename Parameter_t> // a function that returns a Vec3f& to modify.
void add_position(std::shared_ptr<OSSIA::Node> node, Parameter_t&& param)
{
    auto addr = node->createAddress(OSSIA::Value::Type::TUPLE); // [ x y z ]
    auto x_node = *node->emplace(node->children().cend(), "x");
    auto x_addr = x_node->createAddress(OSSIA::Value::Type::FLOAT);
    auto y_node = *node->emplace(node->children().cend(), "y");
    auto y_addr = y_node->createAddress(OSSIA::Value::Type::FLOAT);
    auto z_node = *node->emplace(node->children().cend(), "z");
    auto z_addr = z_node->createAddress(OSSIA::Value::Type::FLOAT);

    addr->setValueCallback([=] (const OSSIA::Value* val) {
        auto tpl = dynamic_cast<const OSSIA::Tuple*>(val);
        if(tpl->value.size() != 3)
            return;

        auto x = dynamic_cast<const OSSIA::Float*>(tpl->value[0]);
        auto y = dynamic_cast<const OSSIA::Float*>(tpl->value[1]);
        auto z = dynamic_cast<const OSSIA::Float*>(tpl->value[2]);
        if(!x || !y || !z)
            return;

        param.set(oalplus::Vec3f{x->value, y->value, z->value});
    });

    auto modify_i = [=] (int i) {
        return [=] (const OSSIA::Value* val) {
            std::cerr << "Got a message!" << std::endl;
            auto float_val = dynamic_cast<const OSSIA::Float*>(val);
            if(!float_val)
                return;

            auto elt = param.get();
            elt[i] = float_val->value;
            param.set(elt);
        };
    };

    x_addr->setValueCallback(modify_i(0));
    y_addr->setValueCallback(modify_i(1));
    z_addr->setValueCallback(modify_i(2));
}

template<typename VecFun> // a function that takes a Vec6f& to modify.
void add_orientation(std::shared_ptr<OSSIA::Node> node, VecFun&& vecfun)
{
    auto addr = node->createAddress(OSSIA::Value::Type::TUPLE); // [ at_x at_y at_z up_x at_y at_z ]
    auto at_node = *node->emplace(node->children().cend(), "at");
    add_position(at_node, [&] () { return vecfun(); } );
    auto up_node = *node->emplace(node->children().cend(), "up");
}

int main(int argc, char** argv)
{
    oalplus::Device device;
    oalplus::ContextMadeCurrent context(
                device,
                oalplus::ContextAttribs().Add(oalplus::ContextAttrib::MonoSources, 1).Get());

    Scene s;

    OSSIA::Local localDeviceParameters{};
    auto local = OSSIA::Device::create(localDeviceParameters, "3DAudioScene");

    auto dev = OSSIA::Device::create(OSSIA::OSC{"127.0.0.1", 9997, 9996}, "MinuitDevice");
    {
        // Listener : position, orientation, volume ?

        // Sources : position, orientation, volume, enabled, sound file ?

        //// Global settings ////
        auto settings_node = *dev->emplace(dev->children().cend(), "settings");
        auto vol_node = *settings_node->emplace(settings_node->children().cend(), "volume");
        auto files_node = *settings_node->emplace(settings_node->children().cend(), "files");

        auto vol_addr = vol_node->createAddress(OSSIA::Value::Type::FLOAT);
        auto files_addr = files_node->createAddress(OSSIA::Value::Type::TUPLE);

        vol_addr->setValueCallback([] (const OSSIA::Value* val) { });
        files_addr->setValueCallback([] (const OSSIA::Value* val) { });

        // updating local tree value
        //OSSIA::Bool b(true);
        //localTestAddress->sendValue(&b);

        //// Listener settings ////
        auto listener_node = *dev->emplace(dev->children().cend(), "listener");

        auto listener_pos_node = *listener_node->emplace(listener_node->children().cend(), "pos");

        add_position(listener_pos_node,
                     make_parameter(
                         [&] () { return s.listener().Position(); },
                         [&] (const auto& elt) { s.listener().Position(elt); }
                     ));

        auto listener_orient_node = *listener_node->emplace(listener_node->children().cend(), "orient");

        //auto addr = listener_orient_node->createAddress(OSSIA::Value::Type::TUPLE); // [ at_x at_y at_z up_x at_y at_z ]
        auto listener_orient_at_node = *listener_orient_node->emplace(listener_orient_node->children().cend(), "at");
        add_position(listener_orient_at_node,
                     make_parameter(
                         [&] () { return s.listener().OrientationAt(); },
                         [&] (const auto& elt) { s.listener().Orientation(elt, s.listener().OrientationUp()); }
                     ));
        auto listener_orient_up_node = *listener_orient_node->emplace(listener_orient_node->children().cend(), "up");
        add_position(listener_orient_up_node,
                     make_parameter(
                         [&] () { return s.listener().OrientationUp(); },
                         [&] (const auto& elt) { s.listener().Orientation(s.listener().OrientationAt(), elt); }
                     ));

    }
    s.start();

    //
    return 0;
}




