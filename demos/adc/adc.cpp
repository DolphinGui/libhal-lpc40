#define BOOST_LEAF_EMBEDDED
#define BOOST_LEAF_NO_THREADS

#include <libarmcortex/dwt_counter.hpp>
#include <libarmcortex/startup.hpp>
#include <libarmcortex/system_control.hpp>
#include <libembeddedhal/counter/util.hpp>
#include <libembeddedhal/serial/util.hpp>
#include <liblpc40xx/adc.hpp>
#include <liblpc40xx/constants.hpp>
#include <liblpc40xx/uart.hpp>

std::span<const std::byte> to_bytes(std::string_view p_view)
{
  return std::as_bytes(std::span<const char>(p_view.data(), p_view.size()));
}

std::span<const std::byte> to_bytes(std::span<char> p_char)
{
  return std::as_bytes(std::span<const char>(p_char.begin(), p_char.end()));
}

int main()
{
  embed::cortex_m::initialize_data_section();
  embed::cortex_m::system_control().initialize_floating_point_unit();

  static embed::cortex_m::dwt_counter counter(
    embed::lpc40xx::internal::get_clock().get_frequency(
      embed::lpc40xx::peripheral::cpu));

  constexpr embed::serial::settings new_settings{ .baud_rate = 38400 };
  auto& adc4 = embed::lpc40xx::adc::get<4>();
  auto& adc2 = embed::lpc40xx::adc::get<2>();
  auto& uart0 = embed::lpc40xx::get_uart<0>(new_settings);
  std::array<char, 32> uptime_buffer{};

  while (true) {
    using namespace std::chrono_literals;

    auto percent_string2 = adc2.read().value().to_string();
    auto percent_string4 = adc4.read().value().to_string();
    auto uptime = embed::this_thread::uptime().count();
    auto result =
      std::to_chars(uptime_buffer.begin(), uptime_buffer.end(), uptime, 10);
    auto uptime_span = to_bytes(std::span(uptime_buffer.begin(), result.ptr));

    (void)embed::write(uart0, to_bytes("("));
    (void)embed::write(uart0, to_bytes(percent_string2));
    (void)embed::write(uart0, to_bytes(", "));
    (void)embed::write(uart0, to_bytes(percent_string4));
    (void)embed::write(uart0, to_bytes("): "));
    (void)embed::write(uart0, uptime_span);
    (void)embed::write(uart0, to_bytes("ns\n"));

    embed::wait_for(counter, 100ms);
  }

  return 0;
}

namespace boost {
void throw_exception(std::exception const& e)
{
  std::abort();
}
}  // namespace boost
