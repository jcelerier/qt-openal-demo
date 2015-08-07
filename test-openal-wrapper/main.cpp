#include <oalplus/al.hpp>
#include <oalplus/all.hpp>
#include <oalplus/alut.hpp>
#include <chrono>
#include <thread>
#include <iostream>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
class Sound
{
    public:
        Sound(const std::string& name):
            Sound{name, oalplus::ALUtilityToolkit(false).CreateBufferHelloWorld()}
        {
        }

        Sound(const std::string& name, const std::string& filename):
            Sound{name, oalplus::ALUtilityToolkit(false).CreateBufferFromFile(filename)}
        {
        }

        Sound(const std::string& name, const oalplus::Buffer& buf):
            m_name{name},
            m_source{oalplus::ObjectDesc(std::string(name))}
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

        void setEnabled(bool b)
        {
            m_source.Gain(b ? m_gain : 0.);
        }

        const std::string& name() const
        {
            return m_name;
        }

        void setName(const std::string &name)
        {
            m_name = name;
        }

        void load(const std::string& file)
        {
            m_source.Buffer(oalplus::ALUtilityToolkit(false).CreateBufferFromFile(file));
        }

    private:
        std::string m_name;
        oalplus::Source m_source;
        double m_gain{1.};
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

            m_sounds.insert(Sound("ASound"));
        }

        void start()
        {
            for(auto& cst_sound : m_sounds)
            {
                const_cast<Sound&>(cst_sound).source().Play();

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

template<typename Fun>
auto add_child(std::shared_ptr<OSSIA::Node>& node,
               const std::string& name,
               OSSIA::Value::Type type,
               Fun&& callback)
{
    auto new_node = *node->emplace(node->children().cend(), name);
    new_node->createAddress(type)->setValueCallback(callback);

    return new_node;
}

template<typename Parameter_t> // a function that returns a Vec3f& to modify.
void add_position(std::shared_ptr<OSSIA::Node> node, Parameter_t&& param)
{
    static auto modify_i = [=] (int i) {
        return [=] (const OSSIA::Value* val) {
            auto float_val = dynamic_cast<const OSSIA::Float*>(val);
            if(!float_val)
                return;

            auto elt = param.get();
            elt[i] = float_val->value;
            param.set(elt);
        };
    };

    auto addr = node->createAddress(OSSIA::Value::Type::TUPLE); // [ x y z ]

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

    add_child(node, "x", OSSIA::Value::Type::FLOAT, modify_i(0));
    add_child(node, "y", OSSIA::Value::Type::FLOAT, modify_i(1));
    add_child(node, "z", OSSIA::Value::Type::FLOAT, modify_i(2));
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
        {
            auto settings_node = *dev->emplace(dev->children().cend(), "settings");
            auto vol_node = *settings_node->emplace(settings_node->children().cend(), "volume");
            auto files_node = *settings_node->emplace(settings_node->children().cend(), "files");

            auto vol_addr = vol_node->createAddress(OSSIA::Value::Type::FLOAT);
            auto files_addr = files_node->createAddress(OSSIA::Value::Type::TUPLE);

            vol_addr->setValueCallback([] (const OSSIA::Value* val) { });
            files_addr->setValueCallback([] (const OSSIA::Value* val) { });
        }
        //// Listener settings ////
        {
            auto listener_node = *dev->emplace(dev->children().cend(), "listener");

            auto listener_pos_node = *listener_node->emplace(listener_node->children().cend(), "pos");

            add_position(listener_pos_node,
                         make_parameter(
                             [&] () { return s.listener().Position(); },
            [&] (const auto& elt) { s.listener().Position(elt); }
            ));

            auto listener_orient_node = *listener_node->emplace(listener_node->children().cend(), "orientation");
            auto listener_orient_addr = listener_orient_node->createAddress(OSSIA::Value::Type::TUPLE); // [ at_x at_y at_z up_x at_y at_z ]
            listener_orient_addr->setValueCallback([&] (const OSSIA::Value* val) {
                auto tpl = dynamic_cast<const OSSIA::Tuple*>(val);
                if(tpl->value.size() != 6)
                    return;

                auto at_x = dynamic_cast<const OSSIA::Float*>(tpl->value[0]);
                auto at_y = dynamic_cast<const OSSIA::Float*>(tpl->value[1]);
                auto at_z = dynamic_cast<const OSSIA::Float*>(tpl->value[2]);
                auto up_x = dynamic_cast<const OSSIA::Float*>(tpl->value[3]);
                auto up_y = dynamic_cast<const OSSIA::Float*>(tpl->value[4]);
                auto up_z = dynamic_cast<const OSSIA::Float*>(tpl->value[5]);
                if(!at_x || !at_y || !at_z || !up_x || !up_y || !up_z)
                    return;

                s.listener().Orientation(at_x->value, at_y->value, at_z->value, up_x->value, up_y->value, up_z->value);
            });

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

        //// Sources settings ////
        {
            auto sources_node = *dev->emplace(dev->children().cend(), "sources");
            auto sources_list_node = *sources_node->emplace(sources_node->children().cend(), "sources");

            add_child(sources_node, "add", OSSIA::Value::Type::STRING,
                      [&] (const OSSIA::Value* val) {
                auto str_val = dynamic_cast<const OSSIA::String*>(val);
                if(!str_val)
                    return;

                // Create the sound
                s.sounds().insert(Sound{str_val->value});
                auto& sound = const_cast<Sound&>(*s.sounds().get<0>().find(str_val->value));

                // Create the callbacks and OSC device commands
                auto src_node = *sources_list_node->emplace(sources_list_node->children().cend(), str_val->value);

                // Position
                auto src_pos_node = *src_node->emplace(src_node->children().cend(), "pos");
                add_position(src_pos_node,
                             make_parameter(
                                 [&] () { return sound.source().Position(); },
                [&] (const auto& elt) { sound.source().Position(elt); }
                ));

                // Enablement
                add_child(src_node, "enabled", OSSIA::Value::Type::BOOL,
                          [&] (const OSSIA::Value* val) {
                    auto enablement_val = dynamic_cast<const OSSIA::Bool*>(val);
                    if(!enablement_val)
                        return;
                    sound.setEnabled(enablement_val->value);
                });

                // Audio file
                add_child(src_node, "file", OSSIA::Value::Type::STRING,
                          [&] (const OSSIA::Value* val) {
                    auto filename_val = dynamic_cast<const OSSIA::String*>(val);
                    if(!filename_val)
                        return;
                    sound.load(filename_val->value);
                });
            });

            add_child(sources_node, "remove", OSSIA::Value::Type::STRING, [&] (const OSSIA::Value* val) {
                auto str_val = dynamic_cast<const OSSIA::String*>(val);
                if(!str_val)
                    return;

                // Remove from the sounds
                s.sounds().erase(str_val->value);

                // Remove from the OSC device
                auto& children = sources_node->children();
                auto it = boost::range::find_if(children,
                                                [&] (auto&& elt) { return elt->getName() == str_val->value; });
                if(it != children.end())
                    children.erase(it);
            });
        }

    }
    s.start();

    return 0;
}
